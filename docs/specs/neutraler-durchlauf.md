# Neutraler spektraler Durchlauf

Status: Offline/Linux-VST3 verifiziert; numerische Abnahme und Windows-Nachweis offen

Spec-Stufe: Spec Anchored

## Ziel und Bedarf

**Belegt:** Musikschaffende sollen das bearbeitete Signal zuverlässig mit dem
Original vergleichen können. Als Entwicklungsgrundlage soll ein neutraler,
überprüfbarer spektraler Durchlauf in einem VST3-Plugin nachgewiesen werden.
Die [Produktvision](../product/product-vision.md#erster-entwicklungs-build)
fordert dafür unter anderem einen latenzkompensierten Nulltest und die korrekte
Latenzmeldung an den Host.

**Vorschlag:** Dieser erste Schritt untersucht den spektralen Signalweg ohne
absichtliche zeitliche Klangbearbeitung. Er ist ein Zwischenstand, nicht der
vollständige erste Entwicklungs-Build.

## Umfang

- **Enthalten:** Neutraler Durchlauf durch den zu prüfenden spektralen
  Verarbeitungsweg, VST3-Host-Anbindung, Latenzmeldung und reproduzierbare
  Messung von Differenz, Pegel und Frequenzverhalten.
- **Nicht enthalten:** Verkürzung oder Verlängerung, editierbare Decay-Kurve,
  Spektralanzeige, Dry/Wet-Mischung, Bypass, Ausgangsschutz und vollständige
  Zustandspersistenz. Diese bleiben Ziele der Vision, werden hier aber nicht
  als erledigt beansprucht.
- **Betroffen:** Audioverarbeitung, Plugin-/Host-Schnittstelle und
  Testumgebung. Eine konkrete Engine-Architektur oder GUI wird nicht festgelegt.
- **Freigegeben am 15.11.2026:** Dieser Prototyp verarbeitet immer neutral
  durch FFT und inverse FFT, ohne Aktivierungsschalter und ohne direkten
  Bypass. Die spätere Kurvenbedienung bleibt außerhalb dieses Auftrags.

## Freigaben aus dem Interview

**Belegt:** Die Produktverantwortung hat Abhängigkeiten nach erfolgreicher
Lizenzprüfung, Versuchsaufbau und Messungen vor verbindlichen Toleranzen
ausdrücklich bestätigt und anschließend die Umsetzung beauftragt.

- JUCE 9.0.5 und PFFFT v1.3.0 an den vollständigen Commits und Archivhashes
  des [Buildmanifests](../../CMakeLists.txt); VST3-SDK 3.8.0 aus JUCE.
  Eigener Code AGPL-3.0-or-later; getrennte Fremdlizenzen siehe
  [Inventar](../../THIRD_PARTY_NOTICES.md).
- FFT 2048, Hop 512, sqrt(periodische Hann) für Analyse und Synthese,
  normiertes WOLA. Versuchskonfiguration, keine Entscheidung über spätere
  Decay-Verarbeitung. Die tatsächliche Latenz muss am Impuls geprüft werden.
- Genau ein Mono- oder Stereo-Ein-/Ausgangsbus, gleiche Layouts, unabhängige
  Kanäle. Kein Mono/Stereo-Upmix oder Downmix.
- Ein 0-Sample-Block ist ein No-op. Reset und erneutes Prepare löschen den
  Zustand; Sampleratenwechsel über erneutes Prepare.
- Nichtendliche Ausgaben sind Fehler. Bei eindeutigem Impulsmaximum muss
  die gemessene der gemeldeten Sample-Latenz exakt entsprechen. Bei
  mehrdeutigen Maxima kein erfolgreicher Latenznachweis.
- Windows-/Ableton-Test später durch die Produktverantwortung nach
  [Anleitung](../verification/windows-ableton.md); genaue Versionen bis
  zur Durchführung offen. Kein solcher Test wurde durchgeführt.

## Messbedingungen

**Für den Versuch freigegeben:** Offline-Tests bei 44,1 und 48 kHz mit
Blockgrößen 1, 64, 512 und 257; Mono und Stereo. Je Test Eingangs- und
Ausgangskanäle getrennt auswerten. Einheitsimpuls, Impulsfolge, Sinus-Sweep,
deterministischer Rauschburst, Stille und Signale nahe Vollaussteuerung.
Signalparameter und Auswertung sind reproduzierbar im Testprogramm
festgelegt; siehe [Prüfprotokoll](../verification/neutral-prototype.md).
Ein- und Ausschwingbereiche sowie die nachfolgend ausgegebene Latenz und
gegebenenfalls Restsignale müssen im Vergleich erfasst werden.

Der Vergleich verwendet den um die **gemeldete** Latenz verzögerten Eingang,
nicht eine frei passend verschobene Referenz. Vor jedem unabhängigen Test wird
der Verarbeitungszustand zurückgesetzt; der Test protokolliert Signal,
Samplerate, Blockgröße, Kanallayout, Messfenster und Einstellung. Die
[Messverfahren](../../.github/skills/audio-verifikation/messverfahren.md) sind
Ausgangspunkt, keine bereits bestätigten Grenzwerte.

## Akzeptanzkriterien

Die Kriterien sind **Entwürfe**. Bedingungen, Toleranzen und offene
Entscheidungen müssen vor einer verbindlichen Abnahme abgestimmt sein. Ein
separat beauftragter Prototyp darf zuvor Messwerte zur Festlegung liefern.

- **AK1 – Neutralität:** Gegeben die abgestimmten Testsignale und
  Messbedingungen sowie der neutrale Zustand, wenn das Signal den spektralen
  Verarbeitungspfad durchläuft, dann bleiben der Spitzen- und RMS-Pegel des
  latenzkompensierten Differenzsignals sowie die Pegel- und
  Frequenzabweichungen innerhalb der abgestimmten Grenzen.
  **Grenzen und Auswertungsverfahren offen;** siehe
  [QS-01](../quality/szenarien.md#qs-01--neutraler-durchlauf).
- **AK2 – Latenz:** Gegeben ein Einheitsimpuls nach dem Zurücksetzen unter den
  abgestimmten Messbedingungen, wenn das Plugin ihn neutral verarbeitet,
  dann entspricht die gemessene Verzögerung in Samples der an den Host
  gemeldeten Latenz. **Versuchsverfahren freigegeben:** Impulse an Start,
  Block-/Hop-/Fenstergrenzen; eindeutiges Betragsmaximum exakt vergleichen,
  mehrdeutige Maxima ausdrücklich als fehlenden Nachweis melden; siehe
  [QS-02](../quality/szenarien.md#qs-02--latenzmeldung).
- **AK3 – Blockgrößen:** Gegeben dasselbe Signal, dieselbe Samplerate und
  derselbe Anfangszustand, wenn mit den abgestimmten Blockgrößen verarbeitet
  wird, dann unterscheiden sich die jeweils latenzkompensierten Ausgaben
  höchstens um die abgestimmte Toleranz. **Toleranz offen.**
- **AK4 – Signalgrenzen:** Gegeben Stille, ein Impuls und ein Signal nahe
  Vollaussteuerung unter den abgestimmten Bedingungen, wenn diese neutral
  verarbeitet werden, dann sind alle Ausgangssamples endlich; Stille erzeugt
  kein Signal oberhalb der abgestimmten Stille-Grenze.
  **Versuchspegel:** −0,1 dBFS; **Stille-Grenze offen.**
- **AK5 – Host-Nachweis:** Gegeben ein lokaler VST3-fähiger Test-Host und ein
  Build für die festgelegte Plattform, wenn das Plugin geladen und ein
  Testsignal im neutralen Zustand verarbeitet wird, dann werden Audio und
  gemeldete Latenz für die Prüfung zugänglich. **Vorgesehen:** der
  JUCE-VST3-Testhost im Repository auf Windows x86-64; tatsächlicher
  Windows-/Ableton-Nachweis noch offen. Der Linux-Test ist kein Ersatz.

Blockgröße 0, Samplerate-Wechsel sowie ein Stereo-Test mit getrennt angeregten
Kanälen gehören zum freigegebenen Versuch; Verhalten siehe oben. Ein Zustand-laden-Test für
Kurven und Parameter gehört nicht zu diesem Zwischenstand.

## Qualität

[QS-01 und QS-02](../quality/szenarien.md) beschreiben Neutralität und
Latenzmeldung. CPU-Grenzen, Höreindruck und DAW-Kompatibilitätsmatrix sind für
diesen Entwurf nicht quantifiziert. Die
[Echtzeitregeln](../../.github/instructions/realtime-audio.instructions.md)
gelten für späteren C++-Audio-Thread-Code; ein Echtzeitnachweis ist hier noch
nicht erbracht.

## Kontext und offene Entscheidungen

- **Belegt:** Die Vision verlangt neutrale Verarbeitung, spektrale
  Verarbeitung, Messung und Host-Latenzmeldung; konkrete Toleranzen sollen
  anhand eines Prototyps festgelegt werden.
- **Freigegeben:** Immer aktiver neutraler STFT-Pfad ohne Kurveneditor;
  kein direkter Bypass und kein Ausgangsschutz in diesem Zwischenstand.
- **Offen – Produktverantwortung mit Test/Entwicklung:** Welche Testmatrix,
  Signalparameter, Auswertungsfenster und Akzeptanzgrenzen werden nach dem
  Prototyp verbindlich? Welche Mono-/Stereo-Layouts gehören dazu?
- **Freigegeben – Produktverantwortung:** STFT als neutraler
  Untersuchungspfad, Windows x86-64 und AGPLv3 als Lizenzrichtung; siehe
  [Architektur](../adr/0001-prototyp-architektur.md) und
  [Lizenzmodell](../adr/0002-lizenzmodell.md).
- **Festgelegt für den Versuch:** Gepinnte Abhängigkeiten und kleiner
  JUCE-VST3-Testhost im Repository. Windows/Ableton-Nachweis ausstehend.
- **Offen – Test/Entwicklung mit Produktverantwortung:** Verbindliche
  numerische Abnahmegrenzen nach den ersten tatsächlichen Messungen.

## Verifikation und Freigabe

- **Geplant:** Deterministische Offline-Messungen für AK1–AK4; Messwerte ohne
  beschlossene Grenzwerte nur berichten, nicht als bestanden ausgeben.
- **Geplant:** VST3-Host-Test für AK2 und AK5; Hörvergleich als ergänzende,
  dokumentierte Prüfung, nicht als Ersatz für den Nulltest.
- **Ausgeführt:** Linux-x86-64-Build sowie Offline-, Processor- und echter
  VST3-Modultest erfolgreich. Gemeldete und am Impuls gemessene Latenz
  2048 Samples. Numerische Ergebnisse bleiben reine Messwerte, keine
  Abnahme offener Toleranzen; siehe
  [Prüfprotokoll](../verification/neutral-prototype.md).
  Windows-/Ableton-Nachweis weiterhin nicht durchgeführt.
- **Echtzeitrestrisiko belegt:** JUCEs VST3-Wrapper nimmt vor dem eigenen
  Callback einen Lock. Kein vollständig lockfreier VST3-Pfad wird behauptet.
- **Freigaben:** Umsetzung und genannte Abhängigkeiten sind beauftragt.
  Neue direkte Abhängigkeiten, Veröffentlichung und externe Aktionen sind
  dadurch nicht pauschal freigegeben.

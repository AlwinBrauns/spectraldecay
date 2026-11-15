# Neutraler spektraler Durchlauf

Status: Entwurf  
Spec-Stufe: Spec First

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
- **Belegt:** In der späteren Kurvenbedienung soll der Wert 0 einen Bereich
  neutral lassen. Für diesen Zwischenstand ist die Aktivierung eines internen
  neutralen Testzustands noch offen. Ein direkter Bypass weist den spektralen
  Durchlauf nicht nach.

## Messbedingungen

**Vorschlag zur Abstimmung:** Offline-Tests bei 44,1 und 48 kHz mit
Blockgrößen 1, 64, 512 und einer festzulegenden ungeraden Größe; Mono und
Stereo, sofern beide Layouts unterstützt werden. Je Test Eingangs- und
Ausgangskanäle getrennt auswerten. Einheitsimpuls, Impulsfolge, Sinus-Sweep,
deterministischer Rauschburst und Stille verwenden; konkrete Pegel, Längen,
Seed, Sweep-Bereich und Messfenster vor einer verbindlichen Abnahme festlegen.
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
  gemeldeten Latenz. **Messposition des Impulses und Vorgehen bei mehrdeutigem
  Maximum offen;** siehe
  [QS-02](../quality/szenarien.md#qs-02--latenzmeldung).
- **AK3 – Blockgrößen:** Gegeben dasselbe Signal, dieselbe Samplerate und
  derselbe Anfangszustand, wenn mit den abgestimmten Blockgrößen verarbeitet
  wird, dann unterscheiden sich die jeweils latenzkompensierten Ausgaben
  höchstens um die abgestimmte Toleranz. **Toleranz offen.**
- **AK4 – Signalgrenzen:** Gegeben Stille, ein Impuls und ein Signal nahe
  Vollaussteuerung unter den abgestimmten Bedingungen, wenn diese neutral
  verarbeitet werden, dann sind alle Ausgangssamples endlich; Stille erzeugt
  kein Signal oberhalb der abgestimmten Stille-Grenze. **Pegel des
  Vollaussteuerungstests und Stille-Grenze offen.**
- **AK5 – Host-Nachweis:** Gegeben ein lokaler VST3-fähiger Test-Host und ein
  Build für die festgelegte Plattform, wenn das Plugin geladen und ein
  Testsignal im neutralen Zustand verarbeitet wird, dann werden Audio und
  gemeldete Latenz für die Prüfung zugänglich. **Host, Plattform und konkrete
  Prüfmethode offen.**

Blockgröße 0, Samplerate-Wechsel sowie ein Stereo-Test mit getrennt angeregten
Kanälen sind zusätzlich zu betrachten. Ihre genauen Bedingungen und
Erwartungen sind vor der Freigabe zu vereinbaren. Ein Zustand-laden-Test für
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
- **Annahme:** Ein früher Testzustand kann ohne fertigen Kurveneditor
  bereitgestellt werden. Ob er intern oder für Nutzende sichtbar ist, ist
  nicht entschieden.
- **Offen – Produktverantwortung:** Wie wird Neutralität aktiviert und wie
  unterscheidet sie sich technisch vom späteren Bypass? Der spätere
  Ausgangsschutz darf ein lautes neutrales Signal verändern; der hier
  untersuchte Signalweg enthält ihn noch nicht.
- **Offen – Produktverantwortung mit Test/Entwicklung:** Welche Testmatrix,
  Signalparameter, Auswertungsfenster und Akzeptanzgrenzen werden nach dem
  Prototyp verbindlich? Welche Mono-/Stereo-Layouts gehören dazu?
- **Freigegeben – Produktverantwortung:** STFT als neutraler
  Untersuchungspfad, Windows x86-64 und AGPLv3 als Lizenzrichtung; siehe
  [Architektur](../adr/0001-prototyp-architektur.md) und
  [Lizenzmodell](../adr/0002-lizenzmodell.md).
- **Offen – Entwicklung mit Produktverantwortung:** Welcher Test-Host und
  welche konkreten Abhängigkeiten, Versionen und Lizenzdetails eignen sich
  für den ersten VST3-Nachweis?
- **Offen – Test/Entwicklung:** Wie werden Nulltest, Frequenzabweichung,
  Impulslatenz, Stille und Blockgrößenvergleich genau ausgewertet?

## Verifikation und Freigabe

- **Geplant:** Deterministische Offline-Messungen für AK1–AK4; Messwerte ohne
  beschlossene Grenzwerte nur berichten, nicht als bestanden ausgeben.
- **Geplant:** VST3-Host-Test für AK2 und AK5; Hörvergleich als ergänzende,
  dokumentierte Prüfung, nicht als Ersatz für den Nulltest.
- **Noch nicht ausführbar:** Es gibt derzeit weder Plugin-Code noch
  Build-System, Tests oder einen ausgewählten VST3-Test-Host. Kein Audio-, Host- oder
  Echtzeitverhalten wurde durch diesen Entwurf nachgewiesen.
- **Freigaben:** Die dokumentierte Architektur- und Lizenzrichtung ist
  freigegeben. Implementierung, konkrete Abhängigkeiten und deren Versionen
  sowie externe Aktionen bedürfen eines gesonderten Auftrags beziehungsweise
  der erforderlichen Zustimmung.

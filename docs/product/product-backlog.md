# Produkt-Backlog: Spectral Decay

Status: Entwurf auf Grundlage der Produktvision und des Interviews vom
15. November 2026. Die Reihenfolge und der Zuschnitt sind Vorschläge, keine
Produktfreigabe. Prototyp 1 ist unter [prototyp/eins](../../prototyp/eins/)
archiviert; im Hauptbaum ist Prototyp 2 mit bipolarem EQ-Peak-Band,
stufenlosem Tone/Noise-Regler und eigenem Editor implementiert. Linux-x86-64
Build und 3/3 CTest-Programme sind belegt. Messwerte und offene Grenzen stehen
im [Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md).
Windows-/Ableton-Hörtest und numerische Produkt-Abnahmetoleranzen bleiben
offen; der Prototyp schließt daher keine Produktanforderung ab.

## Aktueller Umsetzungsstand

| Eintrag | Belegter Stand | Noch offen |
| --- | --- | --- |
| PB-01 | JUCE 9.0.5 und PFFFT v1.3.0 sind im Build fixiert; Lizenztexte und Inventar liegen vor. | Rechteinhaber, abschließende Lizenz-/Distributionsprüfung und Produktarchitektur. |
| PB-02 | Neutraler STFT-Pfad, 2048 Samples gemeldete/gemessene Latenz; Linux-Build und 3/3 CTest-Programme erfolgreich. | Numerische Produkt-Abnahmegrenzen sowie Windows-/Ableton-Nachweis. |
| PB-03 | Prototyp 2 implementiert bipolare Verkürzung mit Onset-Schutz; Offline-Messung zeigt deutlich reduzierte Nachklangenergie bei weitgehend erhaltenem Onset. | Windows-/Ableton-Hörnachweis und numerische Abnahmetoleranzen. |
| PB-04 | Prototyp 2 implementiert Peak-Band-Verlängerung mit tonal/stochastischer Synthese, Morph und begrenztem Sample-Peak-Schutz; siehe [Prüfbericht](../verification/peak-band-prototype-2.md). | Hörqualität, offene Klangtoleranzen und True-Peak-Nachweis. |
| PB-05 | Eigener Peak-Band-Editor mit ziehbarem Punkt und vier gekoppelten Reglern für Prototyp 2. | Windows-/Ableton-Bediennachweis; langfristiger Mehrpunkt-Editor bleibt Produktziel. |
| PB-06 | Mono- und Stereo-Layouts sind implementiert und in den Linux-Tests enthalten. | Produktqualität und Stabilität des Stereobilds unter Bearbeitung sind nicht abgenommen. |
| PB-07 | Kein eigener Bypass und keine Dry/Wet-Mischung. | Latenzkompensierte Mischung, Bypass und Übergänge. |
| PB-08 | P2-Offline-Test weist exakte 2,0-s-Ausgabegrenze und Sample-Peak-Ceiling `max(tailOutputCeiling, |dry|)` nach. | Kein True-Peak-Nachweis; Aktivitätsbezug und Abnahmegrenzen offen. |
| PB-09 | Linux-Build, Offline-, Processor- und geladener VST3-Modultest erfolgreich; Einzelheiten im [P2-Prüfbericht](../verification/peak-band-prototype-2.md). | Windows-/Ableton- und Hörtest; vollständiger Echtzeitnachweis für den JUCE-VST3-Gesamtpfad fehlt. |

Der [neue Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md)
dokumentiert aktuelle Umsetzung, Linux-Messwerte und offene Grenzen. Der
[neutrale Prüfbericht](../verification/neutral-prototype.md) und der
[Tail-Prüfbericht zu P1](../verification/tail-prototype.md) bleiben als
historische Nachweise erhalten. Messwerte sind keine Abnahme offener
Produktgrenzen.

## Übergang zu Prototyp 2

Die berichteten fünf Beobachtungen aus Prototyp 1 begründen einen begrenzten
zweiten Versuch: ein Peak-Band mit `bandFreqHz` und `bandQ`, bipolarer
`bandDecay`-Wert (negativ kürzer, 0 neutral, positiv länger) und stufenloser
`toneNoise`-Übergang. Die Begriffe **Peak-Band**, **Güte / Q**, **bipolarer
Decay-Wert** und **Tone/Noise-Übergang** sind in diesem Versuch gemeint; sie
ändern keine langfristigen Produktziele. Umfang und Kriterien stehen in der
[abgestimmten Prototyp-2-Spec](../specs/prototyp-zwei-peak-band.md).
Zahlen-/Messschwellen und Produktfreigabe bleiben offen.

## Ziel des ersten Entwicklungs-Builds

Musikschaffende können in Ableton Live 12 unter Windows einen experimentellen
VST3-Build auf Mono- und Stereo-Spuren ausprobieren: Verschiedene
Frequenzbereiche lassen sich im selben Signal hörbar verkürzen und
verlängern. Geplante Hörbeispiele sind insbesondere Snare und komplexe
Gemische, außerdem Kick, Vocals und akustische Aufnahmen. Bei Gemischen wirkt
die Bearbeitung auf Frequenzen des gesamten Signals, nicht auf getrennte
Instrumente.

**Bestätigte vorläufige Bedienung:** EQ-ähnliche Kurve mit drei verschiebbaren
Punkten und fließenden Übergängen im Hörbereich; abstrakte Skala −5 bis +5.
0 bedeutet neutral, negative Werte kürzer, positive länger. Die Wirkung soll
stufenlos zunehmen und während der Wiedergabe veränderbar sein. Die Stereo-
Bearbeitung soll gekoppelt sein. Moderate Einstellungen sollen den Anschlag
möglichst erhalten; extreme dürfen ihn verändern. Artefakte sind im ersten
Build akzeptabel.

**Sicherheit und Vergleich:** Zusätzlicher Nachklang soll vorläufig spätestens
zwei Sekunden nach Stille am Eingang ausgeblendet sein. Ein Ausgangsschutz
soll vorläufig höchstens −1 dBTP anstreben und darf auch bei neutraler
Einstellung eingreifen. Bei Bypass und 100 % Dry ist er nicht aktiv. Die
Erkennung von Stille, das Messverfahren und die Übergänge sind offen.

**Ausdrücklich verschoben:** Spektralanzeige und Automation dürfen zunächst
fehlen. Die Projekteinstellungen sollen wiederherstellbar sein; der aktuelle
Prototyp speichert seine vier Versuchsparameter, aber noch keine Decay-Kurve.
Der erste Build ist kein fertiges Release.

## Vorgeschlagene Reihenfolge

### PB-01 – Technische Grundlage und Freigaben

- **Nutzen:** Entwicklung und Prüfung sind auf einer nachvollziehbaren,
  rechtlich geklärten Grundlage möglich.
- **Ergebnis:** Reproduzierbare JUCE-/C++-VST3-Buildgrundlage, Test-Host und
  plattformunabhängiger DSP-Kern; Architektur- und Lizenzrichtung sind
  dokumentiert. Open Source ist angestrebt; Rechteinhaber und
  Distributionsprüfung bleiben offen.
- **Prüfung:** Verantwortliche Person entscheidet über Abhängigkeiten,
  Lizenzmodell, Architektur und Prototypauftrag. Windows-/Ableton-Testweg
  dokumentieren; tatsächlichen Host-Test nicht durch einen Linux-Build
  ersetzen.
- **Vorbereitung:** [Architekturvorschlag](../adr/0001-prototyp-architektur.md)
  und [Lizenzoptionen](../adr/0002-lizenzmodell.md); STFT-Untersuchungspfad,
  AGPLv3-Lizenzrichtung und Windows x86-64 wurden am 15. November 2026
  freigegeben.
- **Status:** Die Prototypgrundlage und die gepinnten Abhängigkeiten sind
  umgesetzt. Rechteinhaber und Distributionsprüfung bleiben offen; daraus
  folgt keine Freigabe für eine Veröffentlichung.

### PB-02 – Neutraler spektraler Durchlauf

- **Nutzen:** Ein verlässlicher Signalweg bildet die Basis für die weitere
  Bearbeitung.
- **Ergebnis:** Spektralen Pfad neutral durchlaufen, tatsächliche Latenz an
  den Host melden und offline messen. Ein direkter Bypass ist kein Nachweis.
- **Prüfung:** [Spec und AK1–AK5](../specs/neutraler-durchlauf.md) sowie
  [QS-01/QS-02](../quality/szenarien.md). Messbedingungen und
  Neutralitätstoleranzen sind noch abzustimmen; vorher Messwerte berichten,
  nicht die Abnahme behaupten.
- **Abhängigkeit:** PB-01 und gesonderter Prototypauftrag.

### PB-03 – Frequenzabhängige Verkürzung

- **Nutzen:** Einzelne Klangbereiche können schneller ausklingen, ohne das
  gesamte Signal gleichermaßen zu bearbeiten.
- **Ergebnis:** Negative Werte bewirken bei geeigneten Signalen im gewählten
  Frequenzbereich eine hörbare Verkürzung; 0 ist neutral. Die Wirkung nimmt
  stufenlos mit dem Abstand von 0 zu.
- **Prüfung:** Deterministische Mehrband- und Transientensignale sowie
  Hörversuch; Messdefinition, Testparameter, Nachweisgrenze und zulässige
  Abweichungen sind offen.
- **Abhängigkeit:** PB-02; konkrete DSP-Entscheidung und eigener Arbeitsauftrag.

### PB-04 – Frequenzabhängige Verlängerung

- **Nutzen:** Ausgewählte Klangbereiche können länger ausklingen.
- **Ergebnis:** Positive Werte bewirken bei geeigneten Signalen eine hörbare
  Verlängerung; Verkürzung und Verlängerung sind gleichzeitig in
  unterschiedlichen Bereichen möglich. Der Nachklang ist gemäß der
  vorläufigen Zwei-Sekunden-Regel begrenzt.
- **Prüfung:** Mehrband-Signal und Hörversuche, darunter Snare und komplexe
  Gemische; Nachklang und Stille gemäß
  [QS-04](../quality/szenarien.md#qs-04--begrenzter-nachklang) messen.
  Messdefinition, Stille-Erkennung und Toleranzen sind offen.
- **Abhängigkeit:** PB-02; eigenständige Untersuchung des hohen
  Verlängerungsrisikos und eigene Architektur-/DSP-Entscheidung. Der
  gleichzeitige Richtungsnachweis setzt auch PB-03 voraus.

### PB-05 – Kurveneditor und Live-Bedienung

- **Nutzen:** Die Wirkung lässt sich direkt und ohne Neustart der Wiedergabe
  einstellen.
- **Ergebnis:** Drei Punkte können in Frequenz und Wert verschoben werden;
  Skala −5 bis +5 mit 0 als neutral, fließende Übergänge und Hörbereich auf
  der Frequenzachse. Änderungen sind während der Wiedergabe hörbar.
- **Prüfung:** Bedienung in Ableton überprüfen; 0, beide Richtungen und
  Zwischenwerte mit Testsignalen prüfen. Frequenzgrenzen, Interpolationsregel,
  Parameterübergänge und Toleranzen sind offen.
- **Abhängigkeit:** PB-01; hörbare vollständige Wirkung benötigt PB-03/PB-04.

### PB-06 – Mono und gekoppeltes Stereo

- **Nutzen:** Einzelspuren und Stereomaterial lassen sich bearbeiten, ohne das
  Stereobild unnötig zu verändern.
- **Ergebnis:** Mono- und Stereo-Layouts funktionieren; Stereo wird
  gekoppelt bearbeitet.
- **Prüfung:** Getrennt angeregte, korrelierte und unkorrelierte Kanäle auf
  Übersprechen und Änderung des Stereobilds untersuchen; Kopplungsverfahren,
  Messgrößen und Grenzen sind offen.
- **Abhängigkeit:** PB-02; Prüfung der bearbeiteten Signale auch mit PB-03/PB-04.

### PB-07 – Dry/Wet und Bypass

- **Nutzen:** Original und Bearbeitung lassen sich vergleichen.
- **Ergebnis:** Dry/Wet und Bypass sind bedienbar; bei Bypass und 100 % Dry
  bleibt der Ausgangsschutz aus. Die Mischung berücksichtigt die
  Verarbeitungslatenz.
- **Prüfung:** Latenzkompensierte Vergleiche bei Bypass und 100 % Dry sowie
  Misch- und Umschalttests; genaue Mischkurve, Bypass-Latenz und zulässige
  Übergangsartefakte sind offen.
- **Abhängigkeit:** PB-02; Schutz-Ausnahmen gemeinsam mit PB-08 prüfen.

### PB-08 – Ausgangsschutz und Nachklangbegrenzung

- **Nutzen:** Der experimentelle Effekt begrenzt Pegelspitzen und endlose
  Nachklänge im vorgesehenen Betriebszustand.
- **Ergebnis:** Aktiver Ausgangsschutz strebt höchstens −1 dBTP an; er kann
  ein neutrales lautes Signal verändern. Bypass und 100 % Dry umgehen ihn.
  Der zusätzliche Nachklang wird nach Stille gemäß der vorläufigen
  Zwei-Sekunden-Regel ausgeblendet.
- **Prüfung:** [QS-03/QS-04](../quality/szenarien.md) mit
  Spitzen-, Transienten- und Stillesignalen; Verfahren, Toleranz,
  Stille-Erkennung und Verhalten beim Umschalten sind offen.
- **Abhängigkeit:** PB-01 für das Schutzkonzept; Nachklangprüfung benötigt
  PB-04, die Prüfung der Ausnahmen PB-07.

### PB-09 – Echtzeit- und Host-Nachweis

- **Nutzen:** Der Build lässt sich beim Abmischen in Ableton erproben.
- **Ergebnis:** VST3 lädt und verarbeitet Audio in Ableton Live 12 unter
  Windows; die tatsächliche Latenz wird gemeldet. Eine von Ableton
  kompensierte höhere Latenz ist für den ersten Einsatz akzeptabel.
- **Prüfung:** Offline-Tests für Blockgrößen, Sampleraten, endliche Samples
  und Latenz; Prüfung der Echtzeitregeln und dokumentierter Windows-/Ableton-
  Hör- und Host-Test. Konkrete CPU-/Latenzgrenzen und Testmatrix sind offen.
- **Abhängigkeit:** PB-01/PB-02; vollständige Build-Abnahme benötigt PB-03 bis
  PB-08.

## Späteres Produktziel

- Spektralanzeige, Punkte hinzufügen und entfernen, weitere
  Punktcharakteristiken und Stereo-Modi.
- Einstellungen im DAW-Projekt speichern und wiederherstellen, Ableton-
  Automation, Presets sowie Qualitäts- und Latenzmodi.
- Natürlicher Klang in moderaten Einstellungen, quantitative
  Qualitätsgrenzen und weitere Plattformen nach Prototypmessungen.

## Noch zu klären

1. **Produktverantwortung / Architektur:** Rechteinhaber, konkrete
   Abhängigkeiten und Versionen sowie Entwicklungs-
   und Testumgebung. Die Windows-x86-64-Umgebung mit Live 12 ist laut
   Produktverantwortung verfügbar.
2. **Produktverantwortung / Test:** geeignete Audio-Messdefinition,
   Testsignale, Messfenster, Neutralitäts- und Richtungstoleranzen; wie stark
   −5 beziehungsweise +5 wirken sollen.
3. **Entwicklung / Test:** Stille-Erkennung und Bezugspunkt für die
   Zwei-Sekunden-Ausblendung, True-Peak-Messverfahren und Toleranz,
   Verhalten an den Schutz-Ausnahmen.
4. **Entwicklung / Test:** Interpolation, Frequenzgrenzen, Stereo-Kopplung,
   Dry/Wet-Mischkurve, Bypass-Latenz und störungsarme Live-Übergänge.
5. **Produktverantwortung:** konkretes Hörtest-Protokoll und Einordnung des
   Blindvergleichs aus [BG-01](../business-goals/business-goals.md#bg-01--ausklänge-gezielt-und-natürlich-formen).

Diese Punkte dürfen durch begrenzte Experimente geklärt werden; offene
Akzeptanzgrenzen sind nicht als bestandene Tests auszugeben.

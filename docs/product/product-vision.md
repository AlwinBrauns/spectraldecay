# Produktvision: Spectral Decay

## Vision

Spectral Decay gibt Musikschaffenden die Möglichkeit, den Ausklang eines Sounds so intuitiv und gezielt zu formen wie seine Frequenzbalance mit einem Equalizer. Einzelne Frequenzbereiche sollen unabhängig voneinander kürzer oder länger ausklingen können, während Charakter, Transienten und räumlicher Eindruck des Originals möglichst erhalten bleiben.

Das Ergebnis soll natürlich und musikalisch klingen – nicht wie ein aufgesetzter Hall oder ein hörbar arbeitendes spektrales Gate.

## Problem und Chance

Klänge bestehen aus Frequenzanteilen mit unterschiedlichen zeitlichen Verläufen. Ein Equalizer verändert vor allem deren Pegelverhältnis; ein herkömmlicher Dynamikprozessor oder Hall bietet dagegen keine gleichermaßen direkte Kontrolle über die Abklingzeit einzelner Frequenzbereiche. Dadurch ist es schwierig, beispielsweise den Bass einer Kick zu verlängern und gleichzeitig ihren hochfrequenten Ausklang zu verkürzen, ohne andere Klangeigenschaften stärker als gewünscht zu verändern.

Spectral Decay schließt diese Lücke mit einer frequenzabhängigen, dynamischen Zeitsteuerung. Im Mittelpunkt steht nicht eine statische Klangfärbung, sondern die gezielte Bearbeitung der zeitlichen Entwicklung des Signals.

## Zielgruppe

- **Musikproduzentinnen und -produzenten**, die Drums, Synthesizer und andere Sounds präzise gestalten möchten.
- **Mixing- und Audio-Engineers**, die störende Resonanzen oder Raumanteile kontrollieren und den Ausklang im Mix formen möchten.
- **Sounddesignerinnen und -designer**, die gezielt neue zeitliche Klangverläufe erzeugen möchten, ohne den gesamten Sound zu verhallen.

## Nutzen und typische Anwendungen

| Anwendung | Gewünschtes Ergebnis |
| --- | --- |
| Kickdrum | Einen dröhnenden Bassausklang verkürzen oder gezielt verlängern. |
| Snare | Den Ausklang straffen oder dem Körper mehr Dauer geben, ohne eine offensichtliche Hallfahne hinzuzufügen. |
| Vocals und Aufnahmen | Störende Resonanz- oder Raumanteile in bestimmten Frequenzbereichen reduzieren. |
| Synthesizer und Sounddesign | Ausgewählte Klanganteile länger tragen lassen und andere gleichzeitig schneller abklingen lassen. |

## Produkterlebnis

Die zentrale Bedienoberfläche ist ein EQ-ähnlicher Kurveneditor. Kontrollpunkte verbinden eine Frequenz mit einer gewünschten Abklingzeit in Millisekunden. Punkte lassen sich hinzufügen, verschieben und entfernen; logarithmische Achsen ermöglichen eine übersichtliche und präzise Bedienung. Eine Spektralanzeige soll die Orientierung am Eingangssignal unterstützen.

Die Bearbeitung reagiert auf das Eingangssignal und verfolgt dessen Ausklang, statt lediglich eine feste Frequenzkurve auf den Pegel anzuwenden. Nutzerinnen und Nutzer können:

- Frequenzbereiche gezielt **verkürzen oder verlängern** und beides gleichzeitig kombinieren.
- Das bearbeitete Signal über **Dry/Wet** mit dem Original mischen und über einen neutralen Bypass vergleichen.
- Die Bearbeitung global steuern und Einstellungen im DAW-Projekt wiederherstellen.
- Stereo-Material bearbeiten, wobei ein gekoppelter Modus das Stereobild möglichst stabil halten soll.

Für eine spätere Ausbaustufe sind unter anderem unterschiedliche Punktcharakteristiken und -breiten, weitere Stereo-Modi, Presets sowie Qualitäts- und Latenzmodi vorgesehen. Diese Funktionen sind Zielbild, keine Aussage über einen bestehenden Implementierungsstand.

## Abgrenzung

Spectral Decay ist weder ein klassischer Lautstärke-EQ noch ein allgemeiner Hallprozessor. Ein EQ-ähnlicher Editor dient als vertrauter Zugang zur **zeitlichen** Bearbeitung. Eine Verlängerung soll den vorhandenen Klangcharakter fortführen, statt einen unabhängigen Raumklang hinzuzufügen. Vollständige Klangneutralität bei jeder Einstellung und auf jedem Eingangssignal ist dabei ein Qualitätsziel, keine Garantie.

## Produktprinzipien

1. **Musikalisch und transparent:** Der bearbeitete Klang soll natürlich bleiben; Transienten, Phase und Stereobild verdienen besondere Aufmerksamkeit.
2. **Gezielt statt pauschal:** Frequenzbereiche sollen unabhängig voneinander und in beide Zeitrichtungen bearbeitet werden können.
3. **Verständlich und direkt:** Eine vertraute Kurvendarstellung und Abklingzeiten in Millisekunden machen die Wirkung nachvollziehbar.
4. **Vergleichbar und kontrollierbar:** Neutraler Durchlauf, Bypass und Dry/Wet erleichtern den direkten Hörvergleich.
5. **Verlässlich im Produktionsalltag:** Das Plugin soll in Echtzeit stabil arbeiten, seine Latenz korrekt an den Host melden und seinen Zustand mit dem DAW-Projekt speichern.

## Erster Entwicklungs-Build

Der erste Entwicklungs-Build soll als VST3-Plugin auf Basis von JUCE und C++ in einer DAW laufen und folgende Kernfunktionen demonstrieren:

- Echtzeit-Spektralanalyse und ein neutraler, überprüfbarer Signaldurchlauf.
- Eine editierbare Decay-Kurve.
- Frequenzabhängiges Verkürzen und Verlängern, auch gleichzeitig in verschiedenen Bereichen.
- Dry/Wet und neutraler Bypass für den direkten Vergleich.

Die Verlängerung soll zunächst moderat und auf maximal zwei Sekunden begrenzt sein. Ein technischer Prototyp darf zunächst schrittweise entstehen; der erste Entwicklungs-Build ist erst dann vollständig, wenn beide Bearbeitungsrichtungen hörbar funktionieren.

## Erfolgskriterien

Das zentrale Anwendungsszenario ist eine Kickdrum, deren Bass länger ausklingt, während die Höhen gleichzeitig schneller abfallen – ohne auffällige Modulation, hörbares Zerfallen oder eine aufgesetzte Hallfahne.

Die Bewertung kombiniert Hörtests mit reproduzierbaren technischen Prüfungen:

- Eingestellte und gemessene Abklingzeiten in geeigneten Testsignalen vergleichen.
- Neutralen Durchlauf auf Pegel- und Frequenzabweichungen sowie per latenzkompensiertem Nulltest prüfen.
- Impulsfolgen, tonale und rauschhafte Signale sowie Stereo-Material auf Transientenverhalten, Artefakte und Stereostabilität untersuchen.
- Bei Verlängerung Spitzenpegel und True Peak beobachten und die Tail-Energie begrenzen.
- Echtzeitverhalten und tatsächliche, an den Host gemeldete Latenz messen.

Konkrete Toleranzen und Akzeptanzschwellen müssen anhand des Prototyps festgelegt werden.

## Technische Richtung und Risiken

Als Grundlage ist eine hybride spektrale Engine vorgesehen: STFT-basierte Analyse und Hüllkurvenverfolgung steuern das Verkürzen; für das Verlängern sollen tonale und rauschhafte Anteile mit geeigneten Verfahren fortgeführt werden. Die konkrete Umsetzung und ihre Qualität sind im Prototyp zu validieren.

Die Verlängerung ist das größte Entwicklungsrisiko: Metallische Artefakte, instabile Phasen, unnatürlich stationäre Klanganteile und überhöhte Pegel können auftreten. Hinzu kommt der Zielkonflikt zwischen Frequenzauflösung, Reaktionszeit, Latenz und Rechenlast. Deshalb stehen eine begrenzte Tail-Energie, moderate erste Einstellungen und reproduzierbare Audio- und Echtzeittests im Vordergrund.

## Entwicklungsweg

1. **Grundlage:** VST3-Host-Anbindung, spektrale Verarbeitung und neutralen Durchlauf nachweisen.
2. **Kernbearbeitung:** Hüllkurven- und Transientenerkennung sowie Verkürzung und Verlängerung entwickeln und testen.
3. **Bedienung:** Kurveneditor, Spektralanzeige, Parameter und weitere vorgesehene Bedienfunktionen ausbauen.
4. **Produktreife:** Audioqualität, CPU-Verbrauch, Latenz und DAW-Kompatibilität optimieren sowie einen Release-Build vorbereiten.

# Glossar: Spectral Decay

Dieses Glossar schafft eine gemeinsame Sprache für Produkt, Entwicklung und Tests.
Es beschreibt die Begriffe aus der [Produktvision](product-vision.md); es legt
keine neuen Anforderungen, Messverfahren oder Akzeptanzschwellen fest. Ein
experimenteller Plugin-Prototyp existiert, bildet aber nur einen Teil des
Produktziels ab.

## Klang und Bearbeitung

| Begriff | Bedeutung im Projekt |
| --- | --- |
| **Ausklang / Decay** | Der zeitliche Verlauf eines Klanganteils nach seinem Anregungs- oder Einschwingvorgang. Er kann in verschiedenen Frequenzbereichen unterschiedlich verlaufen. |
| **Abklingzeit** | Zeitangabe für den Ausklang eines Klanganteils. Ihre genaue Definition und Messung sind noch offen; die Bedienwerte der Decay-Kurve sind keine zugesagten Millisekundenwerte. |
| **Frequenzbereich / Band** | Ein Abschnitt des Frequenzspektrums, dessen zeitlicher Verlauf gezielt bearbeitet oder gemessen wird. Grenzen und Breite sind nicht allgemein festgelegt. |
| **Verkürzung** | Bearbeitung, durch die ein ausgewählter Frequenzbereich schneller abklingt als im Eingangssignal. |
| **Verlängerung** | Bearbeitung, durch die ein ausgewählter Frequenzbereich länger ausklingt als im Eingangssignal. Sie soll vorhandenen Klangcharakter fortführen, keinen unabhängigen Raumklang hinzufügen. |
| **Transient** | Kurzer, schneller Signalvorgang, etwa der Anschlag einer Kickdrum. Sein Verhalten soll bei der Bearbeitung besonders beachtet werden. |
| **Resonanz** | Hervorgehobener, oft länger anhaltender Anteil in einem Frequenzbereich. Die Vision nennt die Kontrolle störender Resonanzen als Anwendung. |
| **Tonaler / rauschhafter Anteil** | Klanganteil mit eher ausgeprägter Tonhöhe beziehungsweise eher breitbandigem, geräuschhaftem Charakter. Beide sollen bei der Verlängerung mit geeigneten Verfahren fortgeführt werden; die Trennung und Verfahren sind offen. |
| **Tail / Nachklang** | Der nach einer Anregung weiter ausklingende Teil des Signals. Bei Verlängerung sind seine Energie und sein Pegel besonders zu beachten. |
| **Tail-Energie** | Maß für die Energie im betrachteten Nachklang. Sie soll begrenzt werden; Messfenster, Berechnung und Grenze sind offen. |
| **Phase** | Zeitliche Lage einer Schwingung relativ zu einer Referenz. Phaseninstabilität kann bei der Verlängerung zu hörbaren Artefakten beitragen. |
| **Artefakt** | Unerwünschte, durch Verarbeitung entstehende Klangveränderung, etwa metallisches Klingen, auffällige Modulation oder hörbares Zerfallen. |

## Bedienung und Signalwege

| Begriff | Bedeutung im Projekt |
| --- | --- |
| **Decay-Kurve** | Im EQ-ähnlichen Editor dargestellte Zuordnung von Frequenzen zu abstrakten Bearbeitungswerten: negativ verkürzt, 0 ist neutral, positiv verlängert. Sie ist keine statische EQ-Pegelkurve. |
| **Kontrollpunkt** | Editierbares Element der Decay-Kurve, das eine Frequenz mit einem Bearbeitungswert verbindet. Für den ersten Build sind vorläufig drei verschiebbare Punkte vorgesehen; Hinzufügen und Entfernen gehören zum späteren Zielbild. |
| **Spektralanzeige** | Darstellung des Eingangsspektrums zur Orientierung im Kurveneditor. Sie ist von der die Bearbeitung steuernden Decay-Kurve zu unterscheiden. |
| **Dry / Wet** | Dry bezeichnet das unbearbeitete, Wet das bearbeitete Signal. Dry/Wet mischt beide für den Vergleich und die Einstellung des Effekts. |
| **Neutraler Durchlauf** | Signalverarbeitung ohne beabsichtigte klangliche Änderung, deren Abweichungen unter anderem mit einem latenzkompensierten Nulltest überprüft werden. |
| **Bypass** | Vergleichszustand, in dem das Signal neutral statt hörbar bearbeitet ausgegeben werden soll. Der Ausgangsschutz ist dabei nicht aktiv; die konkrete Latenzbehandlung ist noch zu spezifizieren. |
| **Gekoppelter Stereo-Modus** | Vorgesehene Bearbeitung von Stereo-Material mit gekoppelten Kanälen, um das Stereobild möglichst stabil zu halten. Art und Grad der Kopplung sind offen. |
| **Stereobild** | Wahrgenommene räumliche Verteilung eines Stereo-Signals. Seine Stabilität ist ein Qualitätsziel. |

## Technik und Verifikation

| Begriff | Bedeutung im Projekt |
| --- | --- |
| **Spektralanalyse** | Untersuchung des Signals nach seinen Frequenzanteilen über die Zeit. Sie dient der geplanten Verarbeitung und der Anzeige. |
| **STFT** | Short-Time Fourier Transform: Analyse eines Signals in aufeinanderfolgenden kurzen Zeitfenstern, um sein zeitabhängiges Spektrum zu bestimmen. Der Prototyp verwendet FFT 2048 und Hop 512; die Konfiguration des späteren Produkts bleibt offen. |
| **Hüllkurve / Hüllkurvenverfolgung** | Die Hüllkurve beschreibt die zeitliche Entwicklung einer Signalgröße; ihre Verfolgung ist als Teil der spektralen Verkürzung vorgesehen. Die genaue Größe und das Verfahren sind offen. |
| **Latenz** | Zeitliche Verzögerung zwischen Eingang und Ausgang durch die Verarbeitung. Die tatsächliche Plugin-Latenz soll korrekt an den Host gemeldet werden. |
| **Latenzkompensierter Nulltest** | Vergleich zweier zeitlich entsprechend ihrer Latenzdifferenz ausgerichteter Signale durch Differenzbildung. Das verbleibende Differenzsignal zeigt Abweichungen; eine Toleranz ist noch offen. |
| **True Peak** | Geschätzter höchster Signalspitzenwert unter Berücksichtigung möglicher Spitzen zwischen digitalen Abtastwerten. Für den aktiven Ausgangsschutz ist vorläufig höchstens −1 dBTP als Ziel vorgesehen. |
| **DAW / Host** | Digital Audio Workstation beziehungsweise die Anwendung, die das Plugin lädt, Audio verarbeitet und Plugin-Zustand und Latenzinformationen verwaltet. |
| **Plugin-Zustand** | Gespeicherte nutzerrelevante Einstellungen, die mit dem DAW-Projekt wiederhergestellt werden sollen, einschließlich der Decay-Kurve. Der aktuelle Prototyp speichert seine vier Versuchsparameter, noch keine Decay-Kurve. |
| **VST3** | Das in der Vision vorgesehene Plugin-Format des ersten Entwicklungs-Builds. |
| **JUCE** | Das in der Vision vorgesehene C++-Framework für den ersten Entwicklungs-Build. |

## Offene Begriffs- und Produktfragen

- **Abklingzeit:** Welcher Pegelabfall, welche Referenz und welches Messverfahren
  eignen sich für den Vergleich der gemessenen Ausklänge? Für welche
  Testsignale und Frequenzbereiche gilt die Messung?
- **Kurvenwirkung:** Wie werden Werte zwischen Kontrollpunkten interpoliert,
  und wie stark wirkt die abstrakte Skala bei unterschiedlichen Eingangssignalen?
- **Verlängerungsgrenze:** Vorläufig soll zusätzlicher Nachklang spätestens
  zwei Sekunden nach Stille am Eingang ausgeblendet sein. Wie werden Stille,
  Ausblendung, Messfenster und Latenzbezug konkret definiert?
- **Nachklang und Pegel:** Über welches Zeitfenster wird Tail-Energie gemessen,
  und welche Grenzen und True-Peak-Anforderungen gelten?
- **Vergleich und Stereo:** Wie werden Dry/Wet und Bypass bei vorhandener
  Latenz genau behandelt, wie verlaufen Übergänge des Ausgangsschutzes und wie
  funktioniert die Stereo-Kopplung?

Diese Fragen sind vor prüfbaren Akzeptanzkriterien zu klären; konkrete
Toleranzen sollen laut Vision anhand des Prototyps festgelegt werden.

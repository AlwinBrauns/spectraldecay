# Qualitätsszenarien

Status: Entwurf. Messbedingungen und verbindliche Freigaben sind offen;
einzelne vorläufige Zielwerte wurden im Produktinterview vereinbart.
Grundlagen sind die [Spec zum neutralen Durchlauf](../specs/neutraler-durchlauf.md)
und das [Produkt-Backlog](../product/product-backlog.md).
Prioritäten sind Vorschläge; konkrete Toleranzen werden laut
[Produktvision](../product/product-vision.md#erfolgskriterien) anhand des
Prototyps festgelegt.

## QS-01 – Neutraler Durchlauf

| Feld | Inhalt |
| --- | --- |
| Qualitätsmerkmal | Funktionale Eignung / Audioqualität |
| Auslöser | Impuls, Impulsfolge, Sinus-Sweep, deterministischer Rauschburst und Stille im neutralen Zustand |
| Umgebung | Offline; vorgeschlagen: 44,1/48 kHz, Blockgrößen 1/64/512 und eine ungerade Größe, Mono/Stereo. Signalparameter, Layouts und Messfenster offen. |
| Artefakt | Spektraler Plugin-Signalweg |
| Erwartete Reaktion | Keine absichtliche Klangänderung; der Signalweg wird tatsächlich durchlaufen. |
| Messgröße | Pro Kanal Spitzen- und RMS-Pegel des latenzkompensierten Differenzsignals sowie Pegel- und Frequenzabweichung; genaue Auswertung offen. |
| Zielwert | Offen – anhand des Prototyps und der Messverfahren festzulegen. |
| Priorität / Risiko | Vorschlag: hoch, da Neutralität die Grundlage für Signal- und Hörvergleiche ist. |
| Prüfung | AK1 und AK4: deterministischer Offline-Test; ergänzend dokumentierter Hörvergleich. |

## QS-02 – Latenzmeldung

| Feld | Inhalt |
| --- | --- |
| Qualitätsmerkmal | Funktionale Eignung / Interoperabilität |
| Auslöser | Einheitsimpuls nach Zurücksetzen im neutralen Zustand |
| Umgebung | Offline und VST3-Test-Host; vorgeschlagene Sampleraten und Blockgrößen wie QS-01. Host und Plattform offen. |
| Artefakt | Spektraler Signalweg und Host-Schnittstelle |
| Erwartete Reaktion | Die an den Host gemeldete Latenz entspricht der tatsächlichen Verzögerung. |
| Messgröße | Differenz in Samples zwischen gemeldeter Latenz und Impulsverzögerung; Impulsposition und Auswertung bei mehrdeutigem Maximum offen. |
| Zielwert | Vorschlag: 0 Samples Abweichung; Messverfahren und Freigabe offen. |
| Priorität / Risiko | Vorschlag: hoch, da eine falsche Meldung Vergleiche und Host-Kompensation verfälscht. |
| Prüfung | AK2 und AK5: Impulsmessung und Vergleich mit der vom Host auslesbaren Meldung. |

## QS-03 – Ausgangsschutz

| Feld | Inhalt |
| --- | --- |
| Qualitätsmerkmal | Audioqualität / Betriebssicherheit |
| Auslöser | Pegelstarke Testsignale und Verlängerung, auch bei neutraler Kurveneinstellung |
| Umgebung | Offline und Windows/Ableton Live 12; Sampleraten, Blockgrößen, Layouts und Messfenster offen |
| Artefakt | Plugin-Ausgang mit aktivem Schutz |
| Erwartete Reaktion | Der Schutz begrenzt Ausgangsspitzen; bei Bypass und 100 % Dry ist er nicht aktiv. |
| Messgröße | Ausgangs-True-Peak in dBTP; geeigneter Messstandard, Messaufbau und Toleranz offen |
| Zielwert | Vorläufig höchstens −1 dBTP, solange der Schutz aktiv ist; keine Zusage für Bypass oder 100 % Dry |
| Priorität / Risiko | Vorschlag: hoch, da Verlängerung und laute Eingangssignale Pegelspitzen verursachen können |
| Prüfung | Synthetische Übersteuerungs- und Transientensignale, Test der Ausnahmen und Übergänge; neutralen Pfad zusätzlich unterhalb der Eingriffsschwelle messen |

## QS-04 – Begrenzter Nachklang

| Feld | Inhalt |
| --- | --- |
| Qualitätsmerkmal | Audioqualität / Betriebssicherheit |
| Auslöser | Verlängerung eines Signals, gefolgt von Stille am Eingang |
| Umgebung | Offline, Mono und Stereo; Signalpegel, Sampleraten, Blockgrößen und Messfenster offen |
| Artefakt | Verlängerung und Nachklangbegrenzung |
| Erwartete Reaktion | Der zusätzliche Nachklang ist spätestens zwei Sekunden nach Stille am Eingang ausgeblendet; ein noch anliegendes Eingangssignal wird dadurch nicht abgeschnitten. |
| Messgröße | Zeit zwischen Eingangsstille und Ende des zusätzlichen Nachklangs; Erkennung von Stille, Endschwelle, Latenzbezug und Toleranz offen |
| Zielwert | Vorläufig maximal zwei Sekunden nach Eingangsstille |
| Priorität / Risiko | Vorschlag: hoch, da unerwünscht lange Nachklänge den Mix beeinträchtigen können |
| Prüfung | Endliche synthetische Signale, anschließende Stille und weiterhin anliegende Signale; Messwerte bis zur Klärung der Bedingungen nur berichten |

## QS-05 – Echtzeitverhalten der STFT-FFT

| Feld | Inhalt |
| --- | --- |
| Qualitätsmerkmal | Leistungseffizienz / Zuverlässigkeit |
| Auslöser | Fortlaufende Verarbeitung jeder STFT-Framegrenze im neutralen Pfad |
| Umgebung | Vorgeschlagen: Windows x86-64 Releasebuild; 44,1 und 48 kHz; Mono und Stereo; Hostblöcke 1, 64, 512 und eine ungerade Größe. DSP-Harness zusätzlich unter Linux. Hardware und Hostmessmethode offen. |
| Artefakt | Plattformunabhängiger DSP-Kern, FFT-Adapter und JUCE-Processor-Callback |
| Erwartete Reaktion | FFT-Setup und Speicher sind vor dem Callback vorbereitet. Im Callback erfolgen keine Heap-Allokationen, Locks, I/O oder Logs; die Arbeit pro Block bleibt begrenzt und die Ausgabe bleibt endlich. |
| Messgröße | Anzahl beobachteter Allokationen/Locks im Callback; Zeit pro Block und Worst-Case-Zeit im Verhältnis zur Blockdauer; Nullrest und Impulslatenz separat gemäß QS-01/QS-02. |
| Zielwert | 0 Callback-Allokationen und 0 Callback-Locks als Muss; CPU-Grenze, Messdauer, Referenzhardware und Neutralitätstoleranz offen – nach dem Prototyp festzulegen. |
| Priorität / Risiko | Vorschlag: hoch, da ein Callback-Lock Dropouts oder unbegrenztes Warten verursachen kann und die FFT den Kern des spektralen Pfads bildet. |
| Prüfung | Statischer Quellreview plus Instrumentierung im freigegebenen FFT-Harness; danach Hosttest. Messwerte ohne abgestimmte CPU-/Nullrestgrenze nur berichten. |

# Qualitätsszenarien

Status: Entwurf. Messbedingungen und verbindliche Freigaben sind offen;
einzelne vorläufige Zielwerte wurden im Produktinterview vereinbart.
Grundlagen sind die [Spec zum neutralen Durchlauf](../specs/neutraler-durchlauf.md)
und das [Produkt-Backlog](../product/product-backlog.md).
Prioritäten sind Vorschläge; konkrete Toleranzen werden laut
[Produktvision](../product/product-vision.md#erfolgskriterien) anhand des
Prototyps festgelegt.

**Implementierungs- und Nachweisstand:** Prototyp 1 ist unter
[`../../prototyp/eins/`](../../prototyp/eins/) archiviert. Prototyp 2 mit
bipolarem Peak-Band und stufenlosem Tone/Noise ist implementiert und unter
Linux-x86-64 mit 3/3 CTest-Programmen geprüft. Messwerte und offene
Toleranzen: [Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md).
Ein qualitativer Ableton-12-/Windows-11-Hörtest von P2 liegt vor, aber ohne
vollständiges Umgebungs-/Materialprotokoll. Formale Host-Abnahme und
numerische Produkt-Abnahmegrenzen bleiben offen; die unten dokumentierten
Szenarien bleiben daher Produktziele, nicht pauschal abgenommene Kriterien.

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

**Aktueller Befund:** Der eigene DSP-Pfad wurde statisch auf die genannten
Regeln geprüft; es gab keine Allokations-, Lock- oder Laufzeitinstrumentierung.
JUCE 9.0.5 sperrt im VST3-Wrapper vor dem eigenen Callback. Das Szenario ist
für den gesamten VST3-Audiopfad damit nicht erfüllt; siehe
[Prüfbericht](../verification/neutral-prototype.md#echtzeit-quellreview-und-restrisiken).

## QS-06 – P1-Tail und berichteter Hörbefund

| Feld | Inhalt |
| --- | --- |
| ID / Qualitätsmerkmal | QS-06 / Audioqualität und Leistungseffizienz |
| Auslöser | Tonaler oder rauschhafter Anteil fällt im ausgewählten Frequenzband ab |
| Umgebung | P1: Auftraggeberbericht über Windows/Ableton-Hörtest am 2026-11-15; verwendete Windows-, Ableton-, Sampleraten-, Blockgrößen- und Signaldetails nicht dokumentiert. Ergänzend vorhandene Offline-/Linux-VST3-Prüfung; diese ersetzt den Windows-Nachweis nicht. |
| Artefakt | Experimenteller P1-Tail-Zweig im STFT-Prototyp |
| Erwartete Reaktion | P1 erzeugt im gewählten Band zusätzliche abklingende Energie. Im berichteten Hörtest klang „Noise“ hallig und „Tone“ granular. Der Befund begründet P2-Untersuchungen, belegt aber weder deren Lösung noch Produktqualität. |
| Messgröße | P1: Tail-RMS/-Peak, Latenz, Blockgrößeninvarianz, Endlichkeit, Stereokanalverhalten und Tail-Ende. P2: zusätzlich Transienten-/Abklingverhalten, Phasen-/Spektralverlauf und dokumentierter Hörvergleich. |
| Zielwert | P1-Versuchsziel: vorläufig spätestens zwei Sekunden nach dem letzten erkannten Signalanteil im jeweiligen Bin. Erkennung, Messbezug sowie sämtliche numerischen Qualitäts- und Artefaktschwellen offen. |
| Priorität / Risiko | Hoch: Verlängerung ist laut Vision das größte Risiko; der berichtete Höreindruck liefert qualitative Evidenz, keine formale Messung. |
| Prüfung | Bestehende P1-Nachweise siehe [Tail-Prüfbericht](../verification/tail-prototype.md). Prototyp-2-Hörtest und Messungen separat dokumentieren; Windows-/Ableton-Konfiguration und Signale beim nächsten Test protokollieren. |

## QS-07 – Bipolarer Decay-Regler

| Feld | Inhalt |
| --- | --- |
| ID / Qualitätsmerkmal | QS-07 / Funktionale Eignung und Audioqualität |
| Auslöser | Änderung von `bandDecay` über negative Werte, 0 und positive Werte |
| Umgebung | Offline-DSP-Test und qualitativer P2-Hörtest in Ableton 12/Windows 11; Samplerate, Blockgröße, Kanallayout und Testmaterial für die formale Wiederholung festzulegen. |
| Artefakt | Bipolare zeitliche Bearbeitung im Peak-Band |
| Erwartete Reaktion | `bandDecay` in `[-5,+5]`: negative Werte verkürzen den Ausklang im Band, 0 ist exakt neutral, positive Werte verlängern ihn. Negative Werte sollen Transienten möglichst erhalten. |
| Messgröße | Latenzkompensierte Differenz zum neutralen Referenzpfad; zeitlicher Zielband-Energieverlauf/Abklingmaß; Transientenpegel; Tail-Dauer und Peak. Messfenster und Verfahren offen. |
| Zielwert | Parameterbereich und Richtungen vorgegeben. P2-Verkürzung wurde bei erhaltenem Attack als zu schwach berichtet; zusätzlicher negativer Bereich wird in P3 untersucht. Neutrale Gleichheit und numerische Mindestwirkung/-toleranzen bleiben offen. |
| Priorität / Risiko | Hoch: bipolarer zeitlicher Eingriff ist eine Kernfunktion; eine statische Dämpfung wäre keine Verkürzung. |
| Prüfung | Deterministischer Kick-/Transienten-Test und abklingender Ton bei negativen, neutralen und positiven Einstellungen; Messwerte berichten und Windows/Ableton gegenhören. Kriterien: [Spec Prototyp 2](../specs/prototyp-zwei-peak-band.md). |

## QS-08 – Stufenloser Tone/Noise-Übergang

| Feld | Inhalt |
| --- | --- |
| ID / Qualitätsmerkmal | QS-08 / Audioqualität |
| Auslöser | Änderung von `toneNoise` von 0 über Zwischenwerte bis 1 während einer Verlängerung |
| Umgebung | Offline-DSP-Render mit abklingendem Sinus und bandbegrenztem Rauschburst; anschließend Windows/Ableton-Hörtest. Pegel, Samplerate und Messfenster offen. |
| Artefakt | Tonale und stochastische Verlängerung sowie deren Morph |
| Erwartete Reaktion | `toneNoise` in `[0,1]` wechselt stetig zwischen peak-gelockter tonaler Synthese bei 0 und gedämpfter stochastischer Synthese bei 1. Gegenüber P1 sollen Granularität bei tonal und hallig/stationär wirkender Noise reduziert werden; vollständige Artefaktfreiheit ist nicht zugesichert. |
| Messgröße | Spektral- und Phasenverlauf, zeitliche Energie/Abklingkurve, Frame-zu-Frame-Sprünge, Peak und dokumentierte Hörbeurteilung über End- und Zwischenwerte. Metriken und Auswertung offen. |
| Zielwert | Numerische Schwellen für Stetigkeit, Energieverhalten und Artefakte ausdrücklich offen – nach Prototyp festzulegen. |
| Priorität / Risiko | Hoch: P1-Hörbericht nennt granulare Tonalität und hallige Noise; keine quantitative Messung bestätigt bisher deren Ursache oder Behebung. |
| Prüfung | Deterministische Sinus- und Rauschbursts getrennt rendern; `toneNoise` an 0, 1 und Zwischenwerten vergleichen; Windows-/Ableton-Hörtest mit dokumentiertem Material. Kriterien: [Spec Prototyp 2](../specs/prototyp-zwei-peak-band.md). |

## QS-09 – EQ-Peak-Band-Steuerung

| Feld | Inhalt |
| --- | --- |
| ID / Qualitätsmerkmal | QS-09 / Funktionale Eignung und Benutzbarkeit |
| Auslöser | Änderung von Mittenfrequenz `bandFreqHz` oder Güte/Breite `bandQ` im Peak-Band-Editor |
| Umgebung | Plugin-Prototyp in Windows/Ableton sowie Offline-DSP-Test; Samplerate, Fenstergröße, Blockgröße und Displayauflösung offen. |
| Artefakt | Peak-Band-Kurve und Frequenzgewichtung der Decay-Bearbeitung |
| Erwartete Reaktion | `bandFreqHz` in `40..16000 Hz` setzt die logarithmische Bandmitte; `bandQ` in `0.25..12.0` verändert die Breite nachvollziehbar. Außerhalb des relevanten Bands klingt die Wirkung stetig ab. |
| Messgröße | Tatsächliche Frequenzantwort der Gewichtungsfunktion gegen gezeichnete Kurve; Lage der Mitte, Bandbreite, Übergang an den Flanken und Parameter-Restaurierung. Bedienbarkeit ergänzend im Hörtest beurteilen. |
| Zielwert | Parameterbereiche vorgegeben; Q-zu-Bandbreiten-Abbildung, numerische Toleranzen, Editorauflösung und Abnahmeschwellen ausdrücklich offen. |
| Priorität / Risiko | Mittel bis hoch: Peak-Band ersetzt zwei als unintuitiv berichtete Frequenzschieber; fehlerhafte Darstellung oder Q-Abbildung kann Kontrolle und Testergebnisse verfälschen. |
| Prüfung | Frequenz-Sweep bzw. gezielte Sinustöne bei mehreren `bandFreqHz`-/`bandQ`-Werten; Kurve mit tatsächlicher Gewichtung vergleichen und im Windows-/Ableton-Test bedienen. Kriterien: [Spec Prototyp 2](../specs/prototyp-zwei-peak-band.md). |

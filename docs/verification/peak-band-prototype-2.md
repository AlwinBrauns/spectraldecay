# Prototyp 2: Bipolares EQ-Peak-Band – Prüfbericht

Stand: 2026-11-20. **Prototyp 2 ist implementiert; Linux-x86-64-Build und
3/3 CTest-Programme sind belegt. Ein informeller Ableton-Live-12-Test unter
Windows 11 liegt als qualitative Rückmeldung vor. Ein reproduzierbares
Hostprotokoll und numerische Produkt-Abnahmetoleranzen bleiben offen.** Der
Bericht dokumentiert einen technischen Versuch, keine Produktfreigabe.

## Überblick und Archivbezug

Prototyp 1 ist nach dem berichteten Windows-/Ableton-Hörtest unter
[`../../prototyp/eins/`](../../prototyp/eins/) konserviert. Prototyp 2 ersetzt
im Hauptbaum dessen Low-/High-Schieber, den unipolaren Tail-Regler und den
binären Tone/Noise-Schalter durch ein einzelnes bipolares EQ-Peak-Band mit
`bandFreqHz`, `bandQ`, `bandDecay` und `toneNoise` sowie einen eigenen
interaktiven Peak-Band-Editor.

Die vorherigen Bedienungs- und Höreindrücke – nur Verlängerung, binärer
Tone/Noise-Modus, zwei Frequenzschieber, „halliges“ Noise und „granularer“
Tone – bleiben als berichtete P1-Beobachtungen erhalten; siehe
[Windows-/Ableton-Prüfplan](windows-ableton.md). Sie sind nicht als formale
P2-Hörmessung zu verstehen.

## Technische Kurzbeschreibung

- STFT mit `N=2048`, `H=512`, periodischem `sqrt(Hann)` und 2048 Samples
  gemeldeter Latenz. Der Processor meldet `engine.latencySamples()` an den
  Host.
- Die logarithmische Peak-Band-Glocke wird von
  `PeakBandStft::computeBandWeight(f, freqHz, q)` berechnet. Sie ist durch
  `bandFreqHz` (40–16000 Hz) und `bandQ` (0,25–12) bestimmt; die konkrete
  Q-/Breitenwahrnehmung bleibt hörend zu prüfen.
- **Verkürzen (`bandDecay < 0`):** stereo-gekoppelte Hüllkurve mit schneller
  und langsamer Referenz, Onset-Schutz und zeitabhängiger Dämpfung. Der
  Transientenbeginn bleibt im Messbeispiel weitgehend erhalten, während der
  spätere Klangkörper stärker reduziert wird.
- **Verlängern (`bandDecay > 0`):** Peak-Phase-Locking für kohärenten,
  tonalen Nachklang (`toneNoise -> 0`). Der stochastische Anteil verwendet
  frequenzabhängige Höhendämpfung und korrelierte Phasen-/Zufallsmodulation
  gegen einen halligen Eindruck (`toneNoise -> 1`). `toneNoise` überblendet
  stufenlos von 0 bis 1.
- Zusätzliche Ausgabe ist spätestens an der 2,0-s-Aktivitätsgrenze exakt
  beendet; ein weicher Fade beginnt am letzten Hop. Der Sample-Peak-Schutz
  begrenzt auf `max(PeakBandStft::tailOutputCeiling, |dry|)`; das Ceiling ist
  0,89125094 (ca. −1 dBFS), kein True-Peak-Nachweis.
- **Editor (`PeakBandAudioEditor`):** logarithmische Frequenzachse
  40–16000 Hz, bipolare Decay-Achse −5 bis +5, gezeichnete Glockenbreite aus
  `bandQ`, ziehbarer Punkt für Frequenz und Decay, Doppelklick setzt Decay
  auf 0, Mausrad ändert `bandQ`. Vier gekoppelte APVTS-Regler steuern
  `bandFreqHz`, `bandQ`, `bandDecay` und `toneNoise`.

Implementierung: [`PeakBandStft.h`](../../src/dsp/PeakBandStft.h),
[`PluginProcessor.h`](../../src/plugin/PluginProcessor.h),
[`PluginProcessor.cpp`](../../src/plugin/PluginProcessor.cpp),
[`PluginEditor.h`](../../src/plugin/PluginEditor.h) und die DSP-/Editor-
Implementierungen neben ihnen.

## Verifikation und Messwerte

Die folgenden Beispiele stammen aus
[`out/offline/offline-metrics.csv`](../../out/offline/offline-metrics.csv)
(300 Messzeilen, jeweils als `MEASUREMENT_ONLY` ausgewiesen) und den
Processor-/Host-Prüfungen. Messwerte belegen das beobachtete Verhalten unter
den jeweiligen Testbedingungen; sie setzen keine offenen Produkttoleranzen
fest.

| Kriterium | Beobachtung / Nachweis | Einordnung |
| --- | --- | --- |
| **AK1 – Neutralität und Latenz** | Gemeldete und gemessene Latenz: **2048 Samples**. Größter Nullrest-Peak: **9,80876e-8 FS = −140,17 dBFS** (48 kHz; bei 44,1 kHz 8,08141e-8 FS). Blockvergleich gegen Block 1: Differenz **0** für 64/257/512. | Offline sowie Processor/VST3-Host messen und vergleichen die Latenz; Nullrest-Toleranz bleibt offen. |
| **AK2 – Verlängerung** | In-Band-Nachklangenergie steigt monoton von neutral über +2,5 zu +5,0, auch neben einem gleichzeitig anliegenden 8-kHz-Dauerton. Beispiel bei 44,1 kHz: neutral 0, **+2,5: 1,6987e-5**, **+5: 4,73653e-4**; bei 48 kHz: **2,97848e-5 < 7,87486e-4** für +2,5/+5. | Richtungswirkung im Offline-Test belegt, Mindestwirkung und Hörqualität offen. |
| **AK3 – Verkürzung und Transientenschonung** | Percussiver Onset bei −5 bleibt bei **86,4903–87,278 %** der neutralen Energie (3,46–3,49× gegenüber der Vergleichsabsenkung auf 25 %). Später Klangkörper: bei −2,5 **42,5305–43,5697 %**, bei −5 **2,1574–2,56422 %** der neutralen Energie. | Starke monotone Nachklangreduktion bei weitgehend erhaltenem Onset in diesem Testsignal; allgemeine Hör- und Toleranzgrenzen offen. |
| **AK4 – Peak-Band-Selektivität** | Verschiebung der Mittenfrequenz von 400 auf 2500 Hz änderte die gemessene Zielwirkung für beide Vorzeichen. Bei 44,1 kHz: +4, Mitten-/verschoben **6,31697e-5 / 2,29737e-13**; −4 **5,83441e-6 / 1,37446e-4**. Off-Mitten-Test bei Q=0,4 gegenüber Q=8: +4 **5,17293e-5 / 1,392e-14**; −4 **9,94122e-6 / 1,38522e-4**. | Wirkung folgt Mittenfrequenz und wird bei höherem Q schmaler; dies validiert nicht die subjektive Q-Kalibrierung. |
| **AK5 – Stufenloser Tone/Noise-Übergang** | Bei 44,1 kHz wachsen die L2-Abstände vom Tone-Endpunkt für `toneNoise` 0/0,25/0,5/0,75/1 monoton: **0 / 0,0111693 / 0,0237421 / 0,038846 / 0,055511**. Die Abstände zum Noise-Endpunkt fallen **0,055511 / 0,0483153 / 0,0356879 / 0,018878 / 0**. Automations-Max-Step bei 48 kHz: **0,0392211**. | Kontinuierliche Bewegung in diesen Messungen; numerische Klick-/Klangtoleranzen offen. |
| **AK6 – 2-s-Grenze und Sample-Peak-Ceiling** | Tail-Differenz ist ab der 2,0-s-Ausgabegrenze exakt 0. Bei 0,85 Eingangsspitze: Ausgangspeak **0,891251**; bei 0,95: **0,95** (höherer Dry-Peak bleibt erhalten). Automation `+5 -> 0`: Max-Step **0,00341334**, danach exakte Stille. | Sample-Peak-Messung, kein True-Peak-Nachweis; Aktivitätsbezug und Abnahmegrenzen bleiben offen. |
| **AK7 – Stereo, Blöcke und Raten** | Offline bei **44,1/48 kHz** und Mono/Stereo, Blöcken **1/64/257/512**: identisches und gegenphasiges Stereo bleibt erhalten; blockweise Differenz 0. Processor-/Host-Prüfungen enthalten zudem 0-Sample- und übergroße Blöcke. Gemessene Mono-Callback-Mittelwerte im CSV **0,441668–263,444 µs**. | Callbackwerte sind Stichproben-Mittelwerte, kein Worst-Case-/Echtzeit- oder CPU-Grenznachweis. |
| **AK8 – Zustand und Editor** | Processor- und VST3-Hosttests prüfen Zustandsversion **2**, Rundlauf der vier Parameter, sicheres Default-Fallback für P1-/leere-/korrupten Zustand, wiederholtes Editoröffnen sowie Ziehen, Doppelklick-Reset und Mausrad für Q. | Programmatische Zustands-/Interaktionschecks bestanden; Windows-/Ableton-Bedienung offen. |

Die drei CTest-Programme `neutral_offline`, `neutral_processor` und
`neutral_vst3_host` bestanden. Der VST3-Hosttest lädt das Linux-Modul. Im
Offline-CSV sind die Zeilen Messprotokoll und nicht selbst ein Nachweis
beschlossener Grenzwerte.

## Ausgeführte Checks und Grenzen

| Check | Ergebnis |
| --- | --- |
| `ctest --test-dir build --output-on-failure` | **3/3 bestanden**: `neutral_offline`, `neutral_processor`, `neutral_vst3_host` (Linux x86-64) |
| Offline-Messwerte | CSV mit 300 `MEASUREMENT_ONLY`-Zeilen; Rohwerte und Testbedingungen siehe oben/verlinkte CSV |
| Windows-x86-64-Build und P2-Hörtest | **Qualitativ in Ableton 12/Windows 11 erprobt; Buildrevision, Umgebung und Testdaten nicht vollständig protokolliert** |
| Verbindliche numerische Neutralitäts-, Klang-, Artefakt-, CPU- und True-Peak-Toleranzen | **Offen** |

JUCE 9.0.5 nimmt im VST3-Wrapper vor dem eigenen Callback einen Callback-Lock.
Das bleibt ein dokumentiertes Echtzeitrisiko; die Linux-Tests sind kein
vollständiger Echtzeitnachweis.

Die qualitative Ableton-Rückmeldung bestätigt den praktischen Nutzen für
Snare-Body, Vocals und Automation. Der Attack blieb beim Verkürzen gut; die
maximale negative Wirkung reichte häufig nicht aus. Tone war passender, aber
noch nicht ausreichend klar; Noise blieb zu hallig und gelegentlich entstanden
Echo-/Granularartefakte. Der nächste Schritt ist der [Prototyp-3-Arbeitsauftrag](../specs/prototyp-drei-zwei-peak-baender.md):
zwei unabhängige Bänder, stärkere attackschonende Verkürzung und ein
P2/P3-Vergleich mit protokollierter Windows-/Ableton-Umgebung.

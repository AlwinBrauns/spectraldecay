# Prototyp 2: Bipolares Peak-Band

Status: umgesetzt (Linux-Tests belegt; qualitativer Ableton-12-/Windows-11-
Hörtest berichtet; formales Hostprotokoll und numerische Abnahmetoleranzen
offen)
Spec-Stufe: Spec First

Implementierungs- und Prüfstand: [Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md);
[DSP](../../src/dsp/PeakBandStft.h),
[Processor](../../src/plugin/PluginProcessor.h) und
[Editor](../../src/plugin/PluginEditor.h). Der Status bezeichnet die
technische Umsetzung und die dokumentierten Linux-Checks, keine
Produktabnahme.

## Ziel und Bedarf

Musikschaffende brauchen für den zweiten experimentellen Prototyp einen intuitiven, einzelnen EQ-artigen Frequenzbereich, dessen Ausklang sich sowohl verkürzen als auch verlängern lässt. Der Prototyp soll die Bedienungs- und Höreindrücke aus Prototyp 1 gezielt untersuchen, ohne daraus eine langfristige Produktentscheidung oder numerische Klangabnahme abzuleiten.

**Belegt – Auftraggeberbericht vom 2026-11-15 über Windows-Build und Ableton-Hörtest von Prototyp 1:**

1. Der Tail ließ sich nur verlängern, nicht verkürzen.
2. Tone/Noise war binär umschaltbar.
3. Zwei Frequenzschieber (Low/High) waren die bisherige Bandbedienung.
4. Noise klang „hallig“.
5. Tone klang „granular“.

Die fünf Punkte sind qualitative, berichtete Beobachtungen. Formale Host-Messreihen, Hörtestprotokoll, Material, Windows-/Ableton-Versionen und Messwerte sind weiterhin offen; siehe [Windows-/Ableton-Prüfplan](../verification/windows-ableton.md). Der archivierte Stand von Prototyp 1 liegt unter [prototyp/eins](../../prototyp/eins/).

**Bezug zur Produktvision:** [Produkterlebnis](../product/product-vision.md#produkterlebnis), [Erster Entwicklungs-Build](../product/product-vision.md#erster-entwicklungs-build) und [Technische Richtung und Risiken](../product/product-vision.md#technische-richtung-und-risiken). Der drei-Punkte-Editor und langfristige Produktziele werden durch diesen Einzelbandversuch nicht ersetzt.

## Umfang

**Enthalten:**
- Bipolare zeitliche Bearbeitung eines Peak-Bands mit neutralem Wert `bandDecay = 0.0`, Verkürzen bei negativen und Verlängern bei positiven Werten.
- Stufenloser Tone/Noise-Parameter für das Verlängerungssignal.
- Ein kompakter EQ-Peak-Band-Editor mit logarithmischer Frequenzachse, vertikaler Richtung „oben = länger / unten = kürzer“, sichtbarer Glockenbreite aus `bandQ`, ziehbarem Punkt, Mausradbedienung für Q und vier gekoppelten Reglern für die vier Parameter.
- Offline-Verifikation, Plugin-/Host-Tests und Windows-/Ableton-Hörprüfung, soweit verfügbar. Numerische Grenzwerte bleiben bis zu gesonderter Festlegung offen.

**Nicht enthalten:** Mehrpunkt-/Mehrband-Editor, automatische Tone/Noise-Klassifikation, ein Produktversprechen für natürliche Artefaktfreiheit, eine feste Millisekunden-Skala, neue Abhängigkeiten oder eine langfristige Engine-/Produktfreigabe. Bypass, Dry/Wet und vollständige Produktreife werden durch diese Spec nicht neu zugesagt.

**Betroffene Komponenten:** DSP-Engine (Peak-Band-Selektion, Verkürzen/Verlängern, Tone/Noise-Morph), vier Parameter, GUI/Editor, Plugin-Zustand, Host-Latenz-/Tail-Anbindung und Offline-/Plugin-Tests.

## Parameter-Modell

| Parameter | Bereich | Default | Bedeutung |
| --- | --- | --- | --- |
| `bandFreqHz` | 40 .. 16000 Hz, log-orientierte Skala mit Skew-Zentrum 1000 Hz | 1000 Hz | Mittenfrequenz des Peak-Bands |
| `bandQ` | 0.25 .. 12.0 | 1.0 | Güte: kleines Q = breit, großes Q = eng/schmalbandig |
| `bandDecay` | -5.0 .. +5.0 | 0.0 | Abstrakter bipolarer Wert: `< 0` verkürzt, `0.0` ist exakt neutral, `> 0` verlängert. Keine Zusage einer Abklingzeit in ms. |
| `toneNoise` | 0.0 .. 1.0 | 0.5 | Stufenloser Übergang: 0.0 = Tone, 1.0 = Noise; Zwischenwerte liegen dazwischen. |

Die Q-zu-Glockenbreite-Abbildung, zeitlichen Kennlinien und sonstige numerische Abnahmeschwellen sind **offen**. Parameterbereiche und Skalenrichtung sind abgestimmt für den Versuch; sie belegen keine Klanggüte.

## Akzeptanzkriterien

- **AK1 – Neutraler Durchlauf:** Gegeben ein deterministisches Testsignal, neutraler `bandDecay = 0.0`, beliebige gültige `bandFreqHz`, `bandQ` und `toneNoise`, Sampleraten 44,1 und 48 kHz sowie Blockgrößen 1, 64, 257 und 512 Samples; wenn das Signal verarbeitet wird, dann entspricht die gemeldete Plugin-Latenz exakt der gemessenen Latenz von 2048 Samples und der latenzkompensierte Nulltest entspricht dem neutralen Referenzpfad. Die Nullrest-Toleranz bleibt **offen**. Ergebnisse müssen unabhängig von Frequenz, Q und Tone/Noise sein.
- **AK2 – Verlängerung:** Gegeben ein abklingendes In-Band-Signal und gleichzeitig ein weiterlaufendes Out-of-Band-Dauersignal; wenn `bandDecay` von 0.0 über +2.5 auf +5.0 erhöht wird, dann steigt die gemessene In-Band-Nachklangenergie monoton, während das Out-of-Band-Signal weiter anliegt. Messfenster, Mindestanstieg und Toleranz sind **offen**.
- **AK3 – Verkürzung:** Gegeben ein transientenhaltiges, abklingendes In-Band-Signal; wenn `bandDecay` von 0.0 über -2.5 auf -5.0 reduziert wird, dann sinkt die In-Band-Nachklangenergie beziehungsweise der Ausklang wird schneller, während der initiale Transient/Onset gegenüber einer bloßen statischen EQ-Pegelabsenkung geschont bleibt. Messfenster, Onset-Metrik und numerische Toleranzen sind **offen**.
- **AK4 – EQ-Peak-Band-Selektivität:** Gegeben Testtöne innerhalb, an den Flanken und außerhalb des Peak-Bands; wenn `bandFreqHz` verschoben und `bandQ` erhöht beziehungsweise verringert wird, dann verschiebt sich der Wirkbereich mit der Mittenfrequenz und ein höheres Q wirkt gegenüber Nachbarfrequenzen schmalbandiger als ein niedriges Q. Das gilt für positive und negative `bandDecay`. Q-Abbildung, Flanken- und Wirkungs-Toleranzen sind **offen**.
- **AK5 – Stufenloser Tone/Noise-Übergang:** Gegeben ein reproduzierbarer Verlängerungs-Test mit `bandDecay > 0`; wenn `toneNoise` auf 0.0, 0.25, 0.5, 0.75 und 1.0 gesetzt wird, dann verändert sich das Verlängerungssignal stetig zwischen phasenkohärent-tonal und rauschhaft, ohne binären Sprung, Klicks oder NaNs. Messgrößen und numerische Stetigkeits-/Klicktoleranzen sind **offen**.
- **AK6 – Nachklanggrenze und Sample-Peak-Ausgangsschutz:** Gegeben Verlängerung und anschließende Eingangsstille, dann ist der zusätzliche Nachklang spätestens 2 s nach der letzten Aktivität des jeweiligen Bins beendet. Bei `bandDecay > 0` gilt für den Sample-Peak-Ausgangsschutz `|output| <= max(tailOutputCeiling, |dry|)`; `tailOutputCeiling` und Übergangs-/Messdetails sind **offen**. Bei `bandDecay <= 0` entsteht nach Stille kein künstlicher Nachklang. Dies ist kein True-Peak-Nachweis.
- **AK7 – Robustheit und Echtzeit:** Gegeben Mono- und Stereo-Testmaterial bei 44,1 und 48 kHz und Blockgrößen 1, 64, 257 und 512; wenn die Verarbeitung mit Parameteränderungen und den Modi läuft, dann bleibt das Stereobild stabil und die Wirkung blockgrößeninvariant innerhalb noch **offener** numerischer Toleranzen. Samples bleiben endlich; im Audio-Thread gibt es keine Allokation, Locks, I/O, Logs oder unbeschränkt lange Arbeit. Callback-Zeit-/CPU-Grenzen sind **offen**.
- **AK8 – Zustand und Editor:** Gegeben gültiger, Prototyp-1-, leerer oder ungültiger Plugin-Zustand; wenn Prototyp 2 den Zustand lädt, speichert und den Peak-Band-Editor öffnet/schließt, dann werden die vier gültigen Parameterwerte gespeichert und wiederhergestellt, und bei altem/leerem/ungültigem Zustand erfolgt ein sicheres Fallback auf gültige Defaults ohne Absturz oder ungültige Samples. Der Editor lässt sich wiederholt öffnen und schließen; GUI und Audio-Thread greifen nicht ungeschützt auf gemeinsame DSP-Daten zu. Kompatibilitätsdetails und numerische Zustands-/GUI-Toleranzen sind **offen**.

## Qualität

Relevante Szenarien: [QS-06 bis QS-09](../quality/szenarien.md) (Tail-/Hörbefund, bipolarer Decay, Tone/Noise-Übergang, Peak-Band-Selektivität). Verbindliche numerische Schwellen bleiben ausdrücklich **offen**.

## Kontext und Evidenz

- **Belegt:** Fünf qualitative Beobachtungen aus dem Auftraggeberbericht vom 2026-11-15. Einzelheiten und Grenzen im [Windows-/Ableton-Prüfplan](../verification/windows-ableton.md) und [Tail-Prüfbericht](../verification/tail-prototype.md).
- **Belegt:** Der archivierte Prototyp-1-Code-/Teststand ist unter [`prototyp/eins/`](../../prototyp/eins/) konserviert.
- **Annahme:** Ein einzelnes Peak-Band und ein bipolarer Parameter sind ein geeigneter, reversibler Versuch, um die berichteten Bedienungs- und Klangprobleme zu untersuchen.
- **Offen:** Messdaten, formale Host- und Umgebungsversionen des berichteten Tests, Nullresttoleranz, Stärke der Richtungswirkung, Messdefinition für Ausklang, Q-zu-Breite-Kalibrierung, Artefakt-/CPU-Schwellen und Produktfreigabe. Entscheider: Produktverantwortung, beraten durch DSP und Test.
- **Maßgebliche Dateien:** [Produktvision](../product/product-vision.md), [Backlog](../product/product-backlog.md), [ADR 0005](../adr/0005-zweiter-prototyp-bipolares-peak-band.md), [QS-06–QS-09](../quality/szenarien.md), [Windows/Ableton](../verification/windows-ableton.md), [Tail-Prüfbericht](../verification/tail-prototype.md).

## Verifikation

- **Automatisiert:** deterministische Offline-Tests für Neutralität, Latenz, Richtungswirkung, Frequenzselektivität, Tone/Noise-Zwischenwerte, Tail-Ende, Sample-Peak, Endlichkeit, Stereo und Blockgrößen; Processor-/VST3-Tests für Parameter und Zustand. AK1–AK8 jeweils gegen ihre beobachtbaren Kriterien prüfen. Numerische Toleranzen nicht aus dem Implementierungsergebnis ableiten, sondern vor formaler Abnahme festlegen.
- **Manuell:** Ein qualitativer Windows-11-/Ableton-12-Hörtest wurde berichtet. Attack und Nutzen waren positiv; Verkürzung war zu schwach, Tone teilweise granular und Noise hallig. Für einen formalen Nachweis die fehlenden Build-, Versions-, Audio-, Samplerate-, Puffer- und Materialdaten bei der nächsten Wiederholung dokumentieren und „granular“/„hallig“ gezielt vergleichen.

## Freigaben

- **Agent darf:** lesen, lokale Dokumente/Quellstände ändern und vereinbarte lokale Builds/Tests ausführen.
- **Braucht Zustimmung:** neue Abhängigkeiten, nicht umkehrbare Architektur-/Produktentscheidung, externe Aktionen. Diese Spec ist die abgestimmte Grundlage für einen experimentellen Prototyp 2; sie erteilt keine Veröffentlichung- oder Produktfreigabe.

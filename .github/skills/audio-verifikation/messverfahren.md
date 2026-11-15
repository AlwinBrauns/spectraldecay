# Testsignale und Messverfahren

Referenz für den Skill `audio-verifikation`. Alle Signale synthetisch erzeugen; keine fremden Samples einchecken.

## Testsignale

| Signal | Zweck | Erzeugung |
| --- | --- | --- |
| Einheitsimpuls | Latenz, Impulsantwort, Transienten | 1,0 an Position n₀, sonst 0 |
| Impulsfolge | Transientenverhalten, Modulation | Impulse im festen Abstand (z. B. 250 ms) |
| Gedämpfter Sinus | Decay-Messung mit bekanntem Soll | `A · e^(−t/τ) · sin(2πft)`; T60 = τ · ln(1000) ≈ 6,91 · τ |
| Mehrton gedämpft | Gleichzeitig verschiedene Bänder | Summe gedämpfter Sinus bei z. B. 60 Hz, 1 kHz, 8 kHz mit unterschiedlichen τ |
| Synthetische Kick | Kern-Szenario der Vision | Sinus mit Pitch-Sweep (z. B. 150→50 Hz) + kurzer Rauschklick, exponentielle Hüllkurve |
| Rauschburst | Rauschhafte Anteile, Verlängerung | Weißes/rosa Rauschen (fester Seed), Gate nach X ms |
| Sinus-Sweep | Frequenzgang im neutralen Zustand | Log-Sweep 20 Hz–20 kHz |
| Stereo-Paar | Stereostabilität | Korreliert (L = R), dekorreliert (unabhängige Seeds), panoramiert |
| Stille / DC / nahe 0 dBFS | Robustheit, Denormals, Clipping | – |

## Abklingzeit messen (pro Band)

1. Ausgang latenzkompensieren.
2. Band isolieren (Bandpass um Zielfrequenz, z. B. Terz- oder Oktavband; Filter selbst hat Einschwingzeit – bei tiefen Frequenzen berücksichtigen).
3. Energie-Abklingkurve per **Schroeder-Rückwärtsintegration**: `E(t) = Σ_{k≥t} x[k]²`, in dB normiert.
4. Lineare Regression im Bereich −5 dB bis −25 dB (T20) oder −5 dB bis −35 dB (T30); auf −60 dB extrapolieren.
5. Rauschboden beachten: Integrationsende vor dem Rauschboden wählen.

**Offen (projektweit zu klären):** Welche Definition „Abklingzeit in ms“ im Plugin bedeutet (T60, Zeit bis −X dB, Zeitkonstante τ). Bis zur Klärung immer angeben, welche Definition gemessen wurde.

## Pegel

- **Sample Peak:** max |x|.
- **True Peak:** Signal 4× überabtasten (z. B. mit Polyphasen-Interpolation), dann max |x| (Ansatz nach ITU-R BS.1770).
- **Tail-Energie:** Energie nach Ende der Eingangsanregung, relativ zur Eingangsenergie in dB.

## Stereo

- Korrelationskoeffizient L/R über Fenster.
- Pegeldifferenz L/R pro Band.
- Mid/Side-Energieverhältnis vor und nach Verarbeitung.

## Numerik

- Alle Ausgangssamples endlich (`std::isfinite`).
- Vergleichstoleranzen für Float-Rechnung bewusst wählen und begründen (z. B. Restpegel in dBFS statt exakter Gleichheit), außer bei Bypass.

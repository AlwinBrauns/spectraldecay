# Spectral Decay – Prototyp 2: Bipolares EQ-Peak-Band

Experimenteller JUCE/C++-VST3-Prototyp, der die Abklingzeit eines einzelnen
Frequenzbands bipolar bearbeitet. Prototyp 2 ersetzt im Hauptbaum die Low-/High-Schieber von Prototyp 1,
den unipolaren Tail-Regler und den binären Tone/Noise-Schalter durch ein
Peak-Band mit **Frequency**, **Width (Q)**, **Decay** und **Tone / Noise**. Der erste Prototyp ist unter [`prototyp/eins/`](prototyp/eins/)
archiviert.

## Bedienung

Der eigene EQ-Peak-Band-Editor stellt Frequenz und Decay als Glockenkurve dar:

- Den **Kurvenpunkt ziehen**, um Mittenfrequenz (logarithmisch, 40–16000 Hz)
  und Decay (`bandDecay`, −5 bis +5) einzustellen. Negative Werte verkürzen,
  0 ist neutral, positive Werte verlängern; der Wert ist keine Zeitangabe in
  Millisekunden.
- Die Glockenbreite wird durch `bandQ` bestimmt (0,25–12); das **Mausrad**
  über der Kurve ändert Q. Ein Doppelklick setzt Decay auf 0.
- Vier gekoppelte Regler stehen für `bandFreqHz` (**Frequency**), `bandQ`
  (**Width (Q)**), `bandDecay` (**Decay**) und `toneNoise` (**Tone / Noise**)
  bereit. Der letzte Regler überblendet stufenlos von 0 (Tone) bis 1 (Noise).

Das ist ein einzelnes experimentelles Peak-Band, kein frei editierbarer
Mehrpunkt-EQ. Dry/Wet und eigener Bypass sind nicht implementiert. Die
Sample-Peak-Begrenzung des zusätzlichen Tails ist kein True-Peak-Limiter.
Einstellungen werden als Zustandsversion 2 gespeichert; ungültige oder alte
Zustände fallen sicher auf Defaults zurück.

## Technischer Aufbau

- STFT: FFT-Größe 2048, Hop 512, periodisches `sqrt(Hann)`, WOLA.
- Gemeldete und gemessene Verarbeitungslatenz: 2048 Samples.
- Bipolare Verkürzung mit stereo-gekoppelter Hüllkurve und Onset-Schutz;
  Verlängerung mit tonalem Peak-Phase-Locking bzw. frequenzgedämpfter,
  korrelierter stochastischer Synthese. Tone/Noise überblendet stufenlos.
- Zusätzliche Ausgabe spätestens ab der 2,0-s-Aktivitätsgrenze exakt beendet;
  Sample-Peak-Schutz `max(tailOutputCeiling, |dry|)`.
- Linux-Offline-Messungen, direkte JUCE-Processor-Tests und ein Testhost, der
  das tatsächliche VST3-Modul lädt.

## Nachweisstand

**Prototyp 2:** Linux-x86-64-Build und alle drei CTest-Programme bestanden:
`neutral_offline`, `neutral_processor` und `neutral_vst3_host`. Gemessene
Latenz **2048 Samples**, größter Nullrest-Peak **9,81e-8 FS (−140,17 dBFS)**;
Blockgrößenvergleich bitgleich für die geprüften Blöcke. Der Onset bleibt im
Percussion-Beispiel bei **86,5–87,3 %**, während die Nachklangenergie bei
`bandDecay = −2,5` auf **42,5–43,6 %** und bei `−5` auf **2,16–2,56 %**
fällt. Die 2-s-Grenze, das Sample-Peak-Ceiling und eine stufenlose Tone/Noise-
Bewegung wurden ebenfalls offline gemessen.

Das sind Messbeispiele und technische Prüfungen, **keine Abnahme offener
numerischer Klangtoleranzen**. Windows-/Ableton-Build und Hörtest von
Prototyp 2 stehen aus; Callback-Zeiten sind Stichproben, kein
Echtzeit-/Worst-Case-Nachweis. JUCE 9.0.5 nimmt im VST3-Wrapper vor dem
Callback einen Lock. Vollständige Messbedingungen, Werte und Grenzen:
[Prüfbericht Prototyp 2](docs/verification/peak-band-prototype-2.md).
Der berichtete Windows-/Ableton-Höreindruck von Prototyp 1 ist separat im
[Windows-/Ableton-Prüfplan](docs/verification/windows-ableton.md) bewahrt.

## Build und Tests

CMake **ab 3.24**, C++17-Compiler und Internetzugang beim ersten Konfigurieren.
Abhängigkeiten sind auf feste Versionen/Quellstände festgelegt. Verifizierter
DSP-Offline-Weg (ohne JUCE und ohne Audio-Gerät):

```sh
cmake --preset offline
cmake --build --preset offline
ctest --preset offline
```

Die Offline-Messwerte werden unter `out/offline/offline-metrics.csv`
geschrieben. `MEASUREMENT_ONLY`-Werte sind Messprotokoll, keine Abnahme
offener Toleranzen.

### Windows x86-64: Visual Studio 2022

Mit Visual Studio 2022, C++-Desktop-Workload und Windows SDK:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
ctest --preset windows-x64
```

### Windows x86-64: VS 2026 und Ninja

Der Visual-Studio-2026-Generator wird von CMake 3.31 nicht erkannt. In einer
x64 Native Tools Command Prompt für VS 2026 (C++-Desktop-Workload, Windows SDK,
Ninja im `PATH`; mit `where cl` x64-Compiler prüfen) den Ninja-Weg verwenden:

```cmd
cmake -S . -B out/windows-2026-x64 -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DSPECTRAL_DECAY_BUILD_PLUGIN=ON
cmake --build out/windows-2026-x64 --parallel 2
ctest --test-dir out/windows-2026-x64 --output-on-failure
```

Der VS-2022-Bundlepfad lautet
`out/windows-x64/SpectralDecay_artefacts/Release/VST3/Spectral Decay Neutral.vst3`;
beim VS-2026/Ninja-Weg liegt es unter
`out/windows-2026-x64/SpectralDecay_artefacts/Release/VST3/Spectral Decay Neutral.vst3`.
Windows-Build und Ableton-Hörtest von Prototyp 2 sind noch nicht belegt.

Nach erfolgreichem Windows-Build und Tests kann das Bundle aus PowerShell im
Repository-Verzeichnis exportiert werden:

```powershell
./scripts/export-plugin.ps1
```

Das Skript verwendet standardmäßig `out/windows-2026-x64`; für das
VS-2022-Preset `-BuildDirectory out/windows-x64` angeben. Export und
manueller Windows-Workflow sind keine Linux-Binärdatei-Umetikettierung.
Details: [Windows-/Ableton-Prüfplan](docs/verification/windows-ableton.md)
und [Exportweg](docs/verification/export.md). Es wurde hier kein
Windows-Workflow ausgelöst und kein Windows-Artefakt erzeugt.

### Zusätzlicher Linux-VST3-Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Die JUCE-GUI-Module benötigen unter Ubuntu die Entwicklungsheader von
X11/Xext/Xrandr/Xinerama/Xcursor/Xi, FreeType und Fontconfig. Die
Linux-VST3-Prüfung belegt nicht die Kompatibilität mit Windows oder Ableton.

## Lizenz und Grenzen

Eigener Code: **AGPL-3.0-or-later**, siehe [LICENSE](LICENSE). JUCE,
VST3-SDK und PFFFT haben getrennte Lizenzbedingungen; siehe
[Drittanbieterinventar](THIRD_PARTY_NOTICES.md).

Es fehlt ein vollständiger Echtzeitnachweis, insbesondere für den JUCE-VST3-
Gesamtpfad mit dessen Wrapper-Lock. Windows-/Ableton-Hörtest von Prototyp 2,
True-Peak-Prüfung, verbindliche numerische Neutralitäts-/Klangtoleranzen und
Produktfreigabe bleiben offen.

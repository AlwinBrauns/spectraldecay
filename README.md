# Spectral Decay: neutraler Prototyp

Experimenteller JUCE/C++-VST3-Quellstand fuer Windows x86-64. Der feste
STFT-Pfad ist noch **kein Decay-Effekt**: keine GUI, Parameter, Dry/Wet,
eigene Bypass-Funktion oder Ausgangsschutz. Nicht auf wichtigen Projekten
einsetzen; vor Hoertests den Abhoerpegel reduzieren.

## Aufbau

- Plattformunabhaengiger DSP-Kern: FFT 2048, Hop 512, sqrt(periodische Hann),
  normiertes WOLA, immer FFT und inverse FFT, Mono/Stereo kanalunabhaengig.
- JUCE-Huelle: Mono-/Stereo-Busse und Sample-Latenzmeldung; keine editierbaren
  Einstellungen, daher noch keine Zustandspersistenz.
- Deterministische Offline-Messungen, direkte Processor-Tests und ein kleiner
  Host, der das tatsaechliche VST3-Modul laedt.

## Build und Tests

CMake **ab 3.24**, C++17-Compiler und Internetzugang beim ersten Konfigurieren.
Quellarchive sind durch Commit und SHA-256 fixiert. Verifizierte DSP-Befehle:

```sh
cmake --preset offline
cmake --build --preset offline
ctest --preset offline
```

Dieser DSP-Lauf benoetigt weder JUCE noch ein Audio-Geraet. Die CSV-Messwerte
entstehen unter `out/offline/offline-metrics.csv`. Offene numerische
Toleranzen werden dadurch **nicht** als bestanden gewertet.

Windows x86-64 mit Visual Studio 2022, C++-Desktop-Workload und Windows SDK:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
ctest --preset windows-x64
```

Alternativ mit Visual Studio 2026 und Ninja (dieser Weg ist fuer dieses
Projekt noch nicht verifiziert): Visual Studio 2026 braucht die Workload
**Desktopentwicklung mit C++** samt Windows SDK; Ninja muss installiert und
im `PATH` sein. CMake 3.31 kennt den Visual-Studio-2026-Generator nicht.
Deshalb eine **x64 Native Tools Command Prompt for VS 2026** oeffnen und mit
`where cl` sicherstellen, dass der Compiler auf x64 zielt (z. B.
`Hostx64\x64\cl.exe`). Das `windows-x64`-Preset nicht fuer diesen Weg
verwenden, da es Visual Studio 2022 auswaehlt:

```cmd
cmake -S . -B out/windows-2026-x64 -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DSPECTRAL_DECAY_BUILD_PLUGIN=ON
cmake --build out/windows-2026-x64 --parallel 2
ctest --test-dir out/windows-2026-x64 --output-on-failure
```

Das Bundle liegt beim VS-2022-Preset unter
`out/windows-x64/SpectralDecay_artefacts/Release/VST3/Spectral Decay Neutral.vst3`,
beim VS-2026/Ninja-Weg unter
`out/windows-2026-x64/SpectralDecay_artefacts/Release/VST3/Spectral Decay Neutral.vst3`.
Keine automatische Installation. Details und manueller Live-Test:
[Windows/Ableton-Test](docs/verification/windows-ableton.md).

Nach erfolgreichem Build und Test kann das Bundle aus PowerShell im
Repository-Verzeichnis exportiert werden:

```powershell
./scripts/export-plugin.ps1
```

Das Skript verwendet standardmaessig `out/windows-2026-x64`; fuer den
VS-2022-Preset den Build-Ordner mit
`-BuildDirectory out/windows-x64` angeben.

Der [Windows-Exportweg](docs/verification/export.md) erzeugt nach erfolgreichen
Tests `plugin-export/spectral-decay.vst3`. Nach einem bewussten Commit/Push
kann der manuelle Workflow **Windows prototype export** ein herunterladbares
ZIP unter **Actions -> Lauf -> Artifacts -> spectral-decay-windows-x64**
bereitstellen. Hier wurde kein Workflow gestartet und noch kein
Windows-Binaerartefakt erzeugt.

Fuer einen zusaetzlichen Linux-VST3-Build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Unter Ubuntu benoetigen die internen JUCE-GUI-Module trotz fehlendem Editor
die Entwicklungsheader von X11/Xext/Xrandr/Xinerama/Xcursor/Xi,
FreeType und Fontconfig. Im Versuch fehlten `libxrandr-dev`,
`libxinerama-dev`, `libxcursor-dev` und `libxi-dev`; nach deren Installation
war der vollstaendige Build erfolgreich.

**Nachweisstand:** Linux-x86-64-Build und **3/3 CTest-Programme erfolgreich**.
Die Offline-Matrix liefert 1438 bestandene harte Strukturchecks und
3776 reine Messzeilen. Gemeldete und gemessene Latenz: **2048 Samples**;
groesster Nullrest-Peak **-127,59 dBFS**, groesster Nullrest-RMS
**-141,74 dBFS**; Blockgroessendifferenz im Versuch **0**.
Diese Messwerte sind keine Abnahme offener Klangtoleranzen.
Ein Windows- oder Ableton-Test wurde nicht durchgefuehrt. Einzelheiten:
[Pruefprotokoll](docs/verification/neutral-prototype.md).

## Lizenz und Grenzen

Eigener Code: **AGPL-3.0-or-later**, siehe [LICENSE](LICENSE).
JUCE, VST3-SDK und PFFFT haben getrennte Lizenzbedingungen:
[Drittanbieterinventar](THIRD_PARTY_NOTICES.md).

Der eigene Audio-Pfad ist auf vorallokierten Speicher und begrenzte Arbeit
ausgelegt. **Kein vollstaendiger Echtzeitnachweis:** Insbesondere nimmt
JUCE 9.0.5 im VST3-Wrapper vor unserem Callback einen Callback-Lock.
Dieser Framework-Lock wird nicht durch einen ungeprueften Vendor-Patch entfernt.
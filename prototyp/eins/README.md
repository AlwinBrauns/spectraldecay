> Konservierter Stand des ersten Prototypen (Prototyp 1). Die Entwicklung
> von Prototyp 2 erfolgt auf Grundlage der Projekt-Dokumentation; dieser
> Archivstand bleibt als Referenz erhalten.

# Spectral Decay: experimenteller Prototyp

Experimenteller JUCE/C++-VST3-Quellstand fuer Windows x86-64. Der bisher
neutrale STFT-Pfad wird um einen begrenzten Verlaengerungsversuch erweitert.
Das ist **noch nicht der vollstaendige Decay-Effekt**: Es gibt weder
Kurveneditor noch Verkuerzung, Dry/Wet, eigene Bypass-Funktion oder
True-Peak-Ausgangsschutz. Nicht auf wichtigen Projekten einsetzen; vor
Hoertests den Abhoerpegel reduzieren.

## Hoerversuch

Nach einem neuen Build das Plugin `Spectral Decay Neutral` in der DAW erneut
laden und das Plugin-Fenster oeffnen. Der Standardeditor bietet:

- **Tail amount:** Staerke des experimentellen Nachklangs; 0 ist neutral.
  Fuer den ersten Versuch den Regler auf 1 stellen.
- **Tail mode:** `Tonal` fuer einen tonalen Klang oder `Noise` fuer einen
  rauschhaften Klang. Vor dem Abspielen einstellen: Ein bereits abgeklungener
  Klang wird nicht nachtraeglich verlaengert.
- **Low frequency / High frequency:** Untere und obere Grenze des
  bearbeiteten Bandes in Hz. Standard: 500 bis 1500 Hz. Der Bereich ist
  zwischen 40 und 16000 Hz waehlbar; vertauschte Grenzen werden sortiert.

Zum Vergleich einen abklingenden Klang mit Anteilen im ausgewaehlten Bereich
abspielen und `Tail amount` zwischen 0 und 1 vergleichen. Der Versuch
verlaengert abfallende Signalanteile im Band, auch wenn anderes Audiomaterial
weiterlaeuft. Er formt keine frei einstellbare Frequenzkurve und
unterscheidet tonale und rauschhafte Anteile nicht automatisch. Die
experimentelle spektrale Huellkurve klingt mit einer Zeitkonstante von
400 ms ab. Die zusaetzliche Energie pro Frequenzbin wird spaetestens zwei
Sekunden nach dem letzten erkannten Signal in diesem Bin beendet; die
Erkennungsschwelle ist eine Versuchsannahme.
Die Einstellungen werden im Plugin-Zustand gespeichert. Eine einfache
Sample-Peak-Begrenzung ist nur bei
hinzugefuegtem Tail aktiv und kein True-Peak-Schutz; laute Eingangssignale
werden nicht grundsaetzlich begrenzt. Klangqualitaet, Ausgangspegel und
DAW-Kompatibilitaet sind nicht abgenommen.

Grundlage ist der [vorgeschlagene Verlaengerungsversuch](../../docs/adr/0004-erster-verlaengerungsprototyp.md).
Anders als in dessen urspruenglichem Offline-Zuschnitt ist diese Erweiterung
bereits im Plugin bedienbar und verfolgt abfallende Bandanteile statt erst
bei Vollbandstille einzusetzen.

## Aufbau

- Plattformunabhaengiger DSP-Kern: FFT 2048, Hop 512, sqrt(periodische Hann),
  normiertes WOLA, immer FFT und inverse FFT; stereogekoppelte
  Band-Huellkurven und Synthese pro Kanal.
- JUCE-Huelle: Mono-/Stereo-Busse, Sample-Latenzmeldung, generischer Editor
  und vier gespeicherte Versuchsparameter.
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
[Windows/Ableton-Test](../../docs/verification/windows-ableton.md).

Nach erfolgreichem Build und Test kann das Bundle aus PowerShell im
Repository-Verzeichnis exportiert werden:

```powershell
./scripts/export-plugin.ps1
```

Das Skript verwendet standardmaessig `out/windows-2026-x64`; fuer den
VS-2022-Preset den Build-Ordner mit
`-BuildDirectory out/windows-x64` angeben.

Der [Windows-Exportweg](../../docs/verification/export.md) erzeugt nach erfolgreichen
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

Unter Ubuntu benoetigen die JUCE-GUI-Module
die Entwicklungsheader von X11/Xext/Xrandr/Xinerama/Xcursor/Xi,
FreeType und Fontconfig. Im Versuch fehlten `libxrandr-dev`,
`libxinerama-dev`, `libxcursor-dev` und `libxi-dev`; nach deren Installation
war der vollstaendige Build erfolgreich.

**Nachweisstand des vorherigen neutralen Builds:** Linux-x86-64-Build und
**3/3 CTest-Programme erfolgreich**. Die Offline-Matrix lieferte 1438
bestandene harte Strukturchecks und
3776 reine Messzeilen. Gemeldete und gemessene Latenz: **2048 Samples**;
groesster Nullrest-Peak **-127,59 dBFS**, groesster Nullrest-RMS
**-141,74 dBFS**; Blockgroessendifferenz im Versuch **0**.
Diese Messwerte sind keine Abnahme offener Klangtoleranzen.
Einzelheiten zum neutralen Build: [Pruefprotokoll](../../docs/verification/neutral-prototype.md).
Der aktuelle Tail-Versuch und seine Grenzen sind separat im
[Pruefbericht zum Tail-Prototyp](../../docs/verification/tail-prototype.md)
dokumentiert.

**Vorherige Erweiterung:** Linux-Build und 3/3 CTest-Programme erfolgreich;
1914 Strukturchecks und 4180 Messzeilen im Offline-Lauf. Tonale und
rauschhafte Testbursts erzeugten einen Tail, auch bei 10 ms Burstlaenge.
Der Nutzer berichtet, dass der erweiterte Prototyp unter Windows laedt,
aber bei seinem Material keine Wirkung zeigte.

**Frequenzwahl und Korrektur:** Linux-Build und 3/3 CTest-Programme
erfolgreich; 2350 Strukturchecks und 4500 Messzeilen im Offline-Lauf.
Bei 48 kHz, Blockgroesse 257 und einem abklingenden 1-kHz-Ton mit
gleichzeitigem 8-kHz-Dauerton ist bei `Tail amount` 1 gegenueber 0 ein
Unterschied messbar; beim bisherigen Stand war er im Testfenster exakt 0.
Geprueft wurden auch zwei Sampleraten, vier Blockgroessen, beide Modi,
Stereo, Zustandsuebernahme und das geladene VST3-Modul. Die offenen
Klangtoleranzen sind damit nicht abgenommen. Windows-/Ableton- und
Hoertest des neuen Stands stehen aus.

Eine stichprobenartige lokale Stereo-Callback-Messung mit aktivem Tail
ueberschritt bei 44,1 kHz und 64 Samples einmal das Blockzeitbudget: 4,13 ms
gegenueber 1,45 ms (vorherige Erweiterung). Im aktuellen Stand wurden bei
48 kHz, 64 Samples, Stereo und aktivem Band 40-16000 Hz bei einem
abklingenden Rauschsignal maximal 0,208 ms CPU-Zeit pro Block gemessen;
das Budget betraegt 1,333 ms. Beide Messungen sind Stichproben auf Linux in
einer geteilten Umgebung; Aussetzer koennen nicht ausgeschlossen werden.
Ein Echtzeit- oder CPU-Grenznachweis fehlt.

## Lizenz und Grenzen

Eigener Code: **AGPL-3.0-or-later**, siehe [LICENSE](../../LICENSE).
JUCE, VST3-SDK und PFFFT haben getrennte Lizenzbedingungen:
[Drittanbieterinventar](../../THIRD_PARTY_NOTICES.md).

Der eigene Audio-Pfad ist auf vorallokierten Speicher und begrenzte Arbeit
ausgelegt. **Kein vollstaendiger Echtzeitnachweis:** Insbesondere nimmt
JUCE 9.0.5 im VST3-Wrapper vor unserem Callback einen Callback-Lock.
Dieser Framework-Lock wird nicht durch einen ungeprueften Vendor-Patch entfernt.
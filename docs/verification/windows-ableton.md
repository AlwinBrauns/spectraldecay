# Windows-x86-64- und Ableton-Live-12-Test

**Status:** Der Nutzer berichtet, dass der neutrale Prototyp unter Windows
gebaut wurde und das Plugin laedt. Ein dokumentierter Windows-/Ableton-Test
der Erweiterung mit Versionsangaben und Hoerpruefung steht noch aus.
Linux-Pruefungen sind kein Ersatz.

## 1. Build und Offline-Hosttest

Vorgeschlagene, noch nicht auf Windows erprobte Umgebung:
CMake 3.31.11 (Projektminimum 3.24), Visual Studio 2022 17.14.44,
C++-Desktop-Workload/MSVC v143 und Windows SDK aus Release 10.0.26100.9457.
Tatsaechliche Compiler- und installierte SDK-Version protokollieren;
SDK-Downloadrelease und Verzeichnisversion koennen sich unterscheiden.
Kein ARM-/32-Bit-Ziel und kein automatisches Installieren der Werkzeuge.

In einer Developer PowerShell im Repository:

```powershell
cmake --version
cl
cmake --preset windows-x64
cmake --build --preset windows-x64
ctest --preset windows-x64
```

Erwartetes Artefakt:
`out/windows-x64/SpectralDecay_artefacts/Release/VST3/Spectral Decay Neutral.vst3`.
Das komplette Bundle verwenden, nicht nur dessen innere Binaerdatei.
Optional erzeugt anschliessend
`./scripts/export-plugin.ps1 -BuildDirectory out/windows-x64` den konsistent
umbenannten Export `plugin-export/spectral-decay.vst3`; siehe
[Download-/Exportweg](export.md). Der manuelle GitHub-Workflow ist hier
nicht gestartet worden; es gibt noch keinen Windows-Download.

CTest umfasst DSP-Messungen, direkte JUCE-Processor-Tests und
`neutral_vst3_host`: Dieser Test scannt und laedt genau dieses Bundle in
einem kleinen JUCE-VST3-Host, liest die vom Wrapper weitergereichte Latenz
und misst die Impulsverzoegerung. Er setzt keinen installierten DAW-Host voraus.
Ein erfolgreicher Test ist noch keine Ableton-Kompatibilitaetszusage.

Aufbewahren: komplettes Build-/CTest-Protokoll sowie
`out/windows-x64/offline-metrics.csv`.
Ein gruener CTest-Lauf bedeutet nur, dass die definierten harten Pruefungen
erfuellt sind, **nicht**, dass offene Neutralitaetstoleranzen abgenommen wurden.

## 2. Ableton Live 12

1. Windows-Build, x86-64, CPU, Plugin-Quellrevision, Compiler/SDK und genaue
   Live-Version notieren. Audiointerface, Treiber, Samplerate, Puffer und
   Plugin-Installationspfad festhalten.
2. Das Bundle manuell in einen von Live gescannten VST3-Ordner kopieren.
   Ggf. Administratorrechte fuer `C:\Program Files\Common Files\VST3`
   separat bestaetigen; dieses Projekt kopiert dorthin nichts automatisch.
3. VST3-Scan aktivieren, scannen und `Spectral Decay Neutral` laden
   (exportierter Bundle-Dateiname: `spectral-decay.vst3`).
   Der JUCE-Standardeditor bietet `Tail amount` und `Tail mode`.
   Nach einem neuen Build das aktualisierte Bundle verwenden und in Live
   bei Bedarf erneut scannen.
4. Bei 44,1 und 48 kHz bekannte Impulse, Sweep und Rauschburst durchleiten.
   Input und Output als verlustfreie Dateien sichern, keine Normalisierung,
   Warp-/Stretch-Bearbeitung, Fades, Send-Effekte oder Master-Bearbeitung.
   Testpegel zunaechst niedrig; **kein True-Peak-Ausgangsschutz** vorhanden.
5. Mono- und Stereo-Quellspuren sowie getrennte L-/R-Anregung verwenden.
   Tatsaechlich ausgehandeltes Plugin-Buslayout separat feststellen:
   Mono-Quellmaterial allein belegt kein Mono-Plugin-Layout.
6. Host-PDC fuer die jeweilige Messung dokumentieren. Ein bereits von Live
   kompensierter Render darf nicht nochmals um die Plugin-Latenz verschoben
   werden. Fuer den samplegenauen Roh-Latenznachweis den obigen Test-Host
   verwenden; eine gerundete Millisekundenanzeige in Live reicht nicht.
7. Audioausgabe, Dropouts/Fehler und Hoereindruck protokollieren. Mit
   Regler 0 neutral vergleichen; dann einen kurzen tonalen Klang bzw.
   Rauschklang mit Anteilen von 500 bis 1500 Hz gefolgt von Stille abspielen
   und mit erhoehtem Tail-Regler und passendem Modus testen. Kein Hoertest
   ersetzt den Nulltest. Der Versuch bietet keine Verkuerzung, keinen
   Kurveneditor, kein Dry/Wet, keinen eigenen Bypass und keinen
   True-Peak-Schutz. Einen Tail-Modus vor dem Abspielen einstellen; ein
   bereits abgeklungener Klang wird nicht nachtraeglich verlaengert.

## Ergebnisvorlage

| Feld | Ergebnis |
| --- | --- |
| Datum, Quellrevision, Windows-Version/x86-64, CPU | ausstehend |
| CMake, Compiler, SDK, Buildtyp | ausstehend |
| Live-Version, Audiointerface/Treiber | ausstehend |
| fs, Blockgroesse, tatsaechliches Buslayout, Testsignal | ausstehend |
| VST3 erkannt/geladen, Audio verfuegbar | nicht getestet |
| Host-gemeldete / am Impuls gemessene Latenz | nicht gemessen |
| PDC-Einstellung, Renderdateien, Nullrest | ausstehend |
| Hoertest, Fehler/Dropouts | nicht getestet |

Fehler und offene Messgrenzen unveraendert berichten, Tests nicht abschwaechen.

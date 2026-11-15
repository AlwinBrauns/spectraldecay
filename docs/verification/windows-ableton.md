# Windows-x86-64- und Ableton-Live-12-Test

**Status (Stand 2026-11-15):** Für Prototyp 1 liegt der berichtete
Windows-/Ableton-Hörbefund unten vor; der Quellstand ist unter
[prototyp/eins](../../prototyp/eins/) archiviert. Prototyp 2 ist inzwischen
implementiert und unter Linux technisch geprüft. Windows-x86-64-Build,
Ableton-Live-12-Test und Hörtest von P2 stehen noch aus; formale Messreihen,
Quellrevision, Versionen, Audiointerface, Samplerate, Puffer, Buslayout und
Testmaterial sind dafür zu protokollieren. Linux-Prüfungen ersetzen diese
Nachweise nicht. Messwerte und offene Grenzen: [Prüfbericht Prototyp 2](peak-band-prototype-2.md).

## Berichteter Prototyp-1-Hoerbefund

Der Auftraggeber berichtete am 2026-11-15 nach Windows-Build und
Ableton-Hoertest von Prototyp 1 folgende fünf Beobachtungen:

1. Der Tail liess sich nur positiv verlaengern, nicht verkuerzen.
2. Tone/Noise war binaer statt stufenlos.
3. Zwei Frequenzschieber (Low/High) dienten der Bandwahl.
4. Noise klang „hallig“.
5. Tone klang „granular“.

Dies sind berichtete Hoereindruecke und Bedienungsbeobachtungen; ein
Testprotokoll mit Messreihen und Versions-/Umgebungsdaten liegt nicht vor. Sie
begründen die Untersuchung in [Spec Prototyp 2](../specs/prototyp-zwei-peak-band.md),
stellen aber weder die formale Host-Abnahme noch eine Produktfreigabe dar.

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
   Prototyp 2 öffnet den eigenen Peak-Band-Editor: Die EQ-Glocke hat eine
   logarithmische Frequenzachse (40–16000 Hz) und eine bipolare Decay-Achse
   (−5 bis +5). Den Kurvenpunkt ziehen, um Mittenfrequenz und kürzer/länger
   einzustellen; Mausrad ändert die Breite `Q`. Vier gekoppelte Regler bieten
   zusätzlich `Frequency`, `Width (Q)`, `Decay` und `Tone / Noise` (0–1,
   stufenlos). Doppelklick auf die Kurve setzt Decay auf 0.
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
7. Audioausgabe, Dropouts/Fehler und Höreindruck protokollieren. Zunächst
   mit `bandDecay = 0` neutral vergleichen; danach einen Kick-/Transienten,
   einen abklingenden Ton und bandbegrenztes Rauschmaterial mit positiven und
   negativen Decay-Werten abhören. Die Bedienung des Kurvenpunkts, Q per
   Mausrad und der stufenlose Tone/Noise-Regler ist Teil des Tests. Auch ein
   weiterlaufendes Signal außerhalb des Peak-Bands verwenden. Gezielt fragen:
   **„Verkürzt `< 0` den Ausklang ohne den Attack zu zerstören?“** und
   **„Klingt Tone weniger granular und Noise weniger hallig?“** Diese Fragen
   prüfen P2-Hypothesen; sie ersetzen weder den latenzkompensierten Nulltest
   noch eine formale Produktabnahme. Prototyp 2 hat keinen Dry/Wet-Regler,
   keinen eigenen Bypass und keinen True-Peak-Nachweis.

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

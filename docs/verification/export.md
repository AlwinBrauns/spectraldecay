# Windows-VST3-Export fuer den spaeteren Download

**Status: Exportweg angelegt, noch kein Windows-Artefakt erzeugt.**

Nach erfolgreichem Windows-x86-64-Build mit aktivierten Tests:

```powershell
./scripts/export-plugin.ps1 -BuildDirectory out/windows-x64
```

Das Skript prueft die drei konfigurierten Testprogramme und fuehrt CTest aus.
Erst danach kopiert es das ganze Bundle nach `plugin-export/spectral-decay.vst3`
und laedt es erneut im VST3-Testhost. Bundle und innere Windows-Binaerdatei
werden konsistent umbenannt; nur den aeusseren Ordner umzubenennen reicht nicht.
Ein vorhandener Export wird nicht still ueberschrieben oder geloescht.
Bei Fehlern ist ein eventuell teilweise kopiertes Bundle kein gueltiger Export.
Das Skript ist hier nicht unter Windows ausgefuehrt worden.

## Download ueber GitHub

Nach einem ausdruecklich veranlassten Commit/Push dieses Quellstands:
**Actions -> Windows prototype export -> Run workflow**.
Der Workflow wird nur manuell ausgeloest; hier wurde kein Lauf gestartet.

Nur nach erfolgreichen Builds, Tests und Export erscheint im Lauf unter
**Artifacts -> spectral-decay-windows-x64** ein ZIP. Darin liegen:

- `spectral-decay.vst3/`: komplettes Windows-x86-64-Bundle mit zentralen Lizenzhinweisen;
- `spectral-decay-source.zip`: eigene Quellen des gebauten Commits;
- `dependency-sources.tar`: entsprechende JUCE-/PFFFT-Quellen mit
  Originaltexten und weiteren Drittanbieterhinweisen;
- CSV-Messwerte, CTest-Log und Quellrevision.

Das ZIP entpacken und das **gesamte** `.vst3`-Bundle zum Testrechner kopieren.
Quellen-/Hinweispakete bei einer Weitergabe nicht weglassen.
Im normalen GitHub-Quellcode-ZIP ist ein nur im Workflow erzeugtes Bundle
nicht automatisch enthalten. Der Workflow veraendert oder committed
keine Repository-Dateien; Artefakte bleiben 14 Tage herunterladbar.
`plugin-export/` wird nicht von Git ignoriert; ein bewusstes spaeteres
Einchecken erfordert trotzdem Build-, Test- und Distributionspruefung.

Verwendete GitHub-Buildwerkzeuge, jeweils MIT und auf Commit fixiert:
`actions/checkout` v4 (`11d5960a326750d5838078e36cf38b85af677262`)
und `actions/upload-artifact` v4 (`ea165f8d65b6e75b540449e92b4886f43607fa02`).
Sie sind CI-Werkzeuge, keine Plugin-Laufzeitabhaengigkeiten.
Die Compiler-/SDK-Ausstattung des Runners `windows-2022` wird erst im Lauf
festgestellt und ist hier nicht als exakt gepinnte Toolchain behauptet.

## Grenzen

Der lokale Build-/Offline-/Hostnachweis unter Linux ist kein Windows-Nachweis.
Ein erfolgreicher Export behauptet keine bestandenen offenen
Neutralitaetstoleranzen, keinen lockfreien JUCE-Gesamtcallback und keinen
Ableton-Test. Siehe [Pruefstand](neutral-prototype.md) und
[Ableton-Anleitung](windows-ableton.md).

Das Skript kopiert die zentralen JUCE-/PFFFT-/VST3-Lizenzhinweise ins Bundle.
Vor Weitergabe muessen auch die uebrigen erforderlichen Texte und die
Korrespondenzquellen einschliesslich Buildanleitung verfuegbar sein.
Das CI-Downloadpaket enthaelt deshalb die Source-Archive; die einzelne
`.vst3`-Dateistruktur allein ist kein vollstaendiges Distributionspaket.

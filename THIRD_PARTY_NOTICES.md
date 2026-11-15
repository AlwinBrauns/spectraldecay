# Lizenz- und Abhaengigkeitsinventar

Stand: 2026-11-15. Quellen und Texte am gepinnten Stand gelesen;
kein Rechtsgutachten und keine abgeschlossene Distributionsfreigabe.

## Eigener Code

Die eigenen Quellen, Tests und Builddateien dieses Prototyps sind durch ihre
SPDX-Kennung unter **AGPL-3.0-or-later** gestellt. Der unveraenderte
AGPLv3-Text liegt in [LICENSE](LICENSE). Ein spaeterer Wechsel der eigenen
Lizenz kann Rechte aller Beitragenden erfordern. Es werden keine unbekannten
Rechteinhaber oder Beitragsrechte erfunden.

## Direkte Komponenten

| Komponente | Fixierter Quellstand | Lizenz / Hinweise |
| --- | --- | --- |
| JUCE 9.0.5 | `d60b7a78030661574a615b586d9325ba7a7fa7dd` | `AGPL-3.0-only OR LicenseRef-JUCE-Commercial`; ausschliesslich AGPLv3-Pfad gewaehlt, kein kommerzieller Tarif abgeschlossen |
| PFFFT v1.3.0 | `179a48d486cbee8ec5416e5c163ad575c2efe1f1` | BSD-artiger UCAR/NCAR-Text; Copyright, Bedingungen und Haftungsausschluss bei Source-/Binaerweitergabe erhalten; kein Endorsement |
| VST3-SDK, in JUCE enthalten | 3.8.0, Bestandteil desselben JUCE-Commits | MIT; Steinberg-Copyright und Lizenztext erhalten; Markenregeln separat |

Die PFFFT-Tagreferenz ist ein annotiertes Tag-Objekt
`c618791045b1949738fa938cff44a7b0b61585b8`; der oben genannte
Commit ist das aufgeloeste Ziel. Beide sind nicht zu verwechseln.
Der PFFFT-Text enthaelt abweichende Formulierungen gegenueber dem
kanonischen BSD-3-Clause-Text. Deshalb wird hier **kein unbestaetigter
SPDX-Identifier** fuer PFFFT vergeben.

Die Archiv-SHA-256 stehen im [Buildmanifest](CMakeLists.txt).
Es werden nur `pffft.c` und `pffft_common.c` mit ihren zugehoerigen Headern
kompiliert; keine Benchmark-FFTs oder weiteren PFFFT-Bibliotheken.
Die Archive werden ohne fremde Git-Submodule bezogen.

JUCEs `AGPL-3.0-only` aendert nicht die Kennung eigener Dateien.
Die Kombination nutzt AGPL Version 3; eine automatische Berechtigung,
JUCE unter kuenftigen AGPL-Versionen zu nutzen, wird nicht behauptet.

## JUCE-Transitiva

Der VST3-Client benoetigt trotz fehlendem eigenen Editor JUCE-GUI-Module.
Verwendete Modulrichtung: `juce_audio_processors`, `juce_audio_processors_headless`,
`juce_audio_plugin_client`, `juce_gui_extra`, `juce_gui_basics`,
`juce_graphics`, `juce_events`, `juce_data_structures`, `juce_audio_basics`,
`juce_core`. Der separate Test-Host aktiviert nur VST3-Hosting.
Webbrowser/CURL sind fuer die eigenen Ziele ausgeschaltet. Keine
Audio-Geraete-, ASIO-, AAX-, LV2- oder VST2-Ziele werden angefordert.

Im versionsgebundenen JUCE-Inventar und den zugehoerigen Modulquellen:

| Bestandteil | Version | Lizenz laut JUCE-SBOM |
| --- | --- | --- |
| zlib | 1.3.2 | Zlib |
| HarfBuzz | 15.0.0 | MIT-Modern-Variant |
| SheenBidi | 2.9.0 | Apache-2.0 |
| LunaSVG | 3.5.0 | MIT |
| PlutoVG | 1.3.2 | MIT |
| libpng | 1.6.58 | libpng-2.0 |
| jpeglib | 10.0 | IJG |
| libwebp | 1.6.0 | BSD-3-Clause |
| Chromium cubic_bezier | `3e1a26c44c024d97dc9a4c09bbc6a2365398ca2c` | BSD-3-Clause |

Dies ist ein **Quell-/Modulinventar**, kein verifiziertes Inventar eines
fertigen Windows-Binaerartefakts. Plattformbibliotheken (unter Linux etwa
FreeType/Fontconfig/X11, unter Windows Systembibliotheken und Compiler-
Runtime) sind am tatsaechlichen Build zusaetzlich zu erfassen.

## Originaltexte und Distribution

Nach Konfiguration sind die Originaltexte verfuegbar unter:

- `build/_deps/pffft-src/LICENSE.txt` und den Headern/Quellen.
- `build/_deps/juce-src/LICENSE.md` und `JUCE.spdx.json`.
- `build/_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt`.
- Den im SBOM aufgefuehrten eingebetteten Lizenzdateien, z. B. HarfBuzz
  `COPYING`, SheenBidi `LICENSE`, LunaSVG/PlutoVG `LICENSE`, libwebp `COPYING`
  und den Lizenzkommentaren der zlib-/JPEG-/PNG-Quellen.

Bei Presets liegt derselbe `_deps`-Baum im jeweiligen `out/...`-Buildordner.
Vor Binaerverteilung sind alle tatsaechlich erforderlichen Texte/Notices
mitzuliefern und die AGPL-Quellbereitstellung einschliesslich Buildanleitung,
entsprechender JUCE-/PFFFT-Quellen und etwaiger Aenderungen sicherzustellen.
Ein Link allein ersetzt keine erforderliche mitzuliefernde Lizenzkopie.
Der Prototyp installiert oder veroeffentlicht keine Binaerdateien automatisch.

Quellen:
[JUCE-Lizenz](https://github.com/juce-framework/JUCE/blob/9.0.5/LICENSE.md),
[JUCE-SBOM](https://github.com/juce-framework/JUCE/blob/9.0.5/JUCE.spdx.json),
[PFFFT-Lizenz](https://github.com/marton78/pffft/blob/v1.3.0/LICENSE.txt),
[VST3-MIT-Text](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt).

## Verifizierter lokaler Build und CI-Werkzeuge

Der Linux-Build hat die oben genannten JUCE-Module und ihre Grafik-/Text-
Quellen tatsaechlich kompiliert. Fehlende Xrandr-/Xinerama-/Xcursor-/Xi-
Entwicklungsheader wurden ueber die vorhandenen Ubuntu-Paketquellen
installiert. Das ersetzt keine Windows-Binaerinventarisierung.

Der ausschliesslich manuell gestartete Windows-Exportworkflow verwendet
`actions/checkout` v4 am Commit `11d5960a326750d5838078e36cf38b85af677262`
und `actions/upload-artifact` v4 am Commit
`ea165f8d65b6e75b540449e92b4886f43607fa02`, beide unter MIT.
Diese sind CI-Werkzeuge, keine eingebundenen Plugin-Laufzeitbibliotheken.
PowerShell wird fuer Export/Validierung verwendet, nicht mit dem Plugin verlinkt.

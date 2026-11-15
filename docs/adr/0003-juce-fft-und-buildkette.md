# 0003 – JUCE-Version, FFT und Buildkette für den neutralen Prototyp

- **Status:** vorgeschlagen
- **Datum:** 2026-11-15
- **Entscheider:** Produktverantwortung; Lizenzfragen erforderlichenfalls mit Rechtsberatung
- **Bezug:** [0001 – Architektur für den neutralen Prototyp](0001-prototyp-architektur.md), [0002 – Lizenzoptionen](0002-lizenzmodell.md), [Spec neutraler Durchlauf](../specs/neutraler-durchlauf.md), [QS-01/QS-02/QS-05](../quality/szenarien.md)

## Entscheidungsfrage und Kontext

Welche konkrete JUCE-Version und FFT-Implementierung eignen sich als
Freigabevorschlag für einen neutralen, plattformunabhängigen STFT-DSP-Kern
und einen Windows-x86-64-VST3-Prototyp? Welche Buildwerkzeuge sollen für
reproduzierbare Windows-Builds festgehalten werden?

**Belegt:** ADR 0001 hat STFT als Untersuchungspfad und Windows x86-64
freigegeben, aber FFT, Fenster, Hop und Versionen offengelassen. ADR 0002 hat
AGPLv3 als Lizenzrichtung freigegeben, aber weder eine JUCE-Version noch
deren konkreten Lizenzpfad festgelegt.

**Muss-Anforderungen für diesen Vorschlag:**

- JUCE/C++-VST3 für Windows x86-64; DSP-Kern unabhängig von JUCE-Processor,
  Host und GUI testbar.
- Kein Heap-Wachstum, kein Lock, kein I/O und kein Logging im Audio-Callback;
  Arbeit und Speicherbedarf müssen vorab begrenzt und vorbereitbar sein.
- Ein neutraler STFT-Pfad muss offline mit latenzkompensiertem Nulltest,
  Impuls- und Blockgrößentests prüfbar sein.
- Die konkrete JUCE- und FFT-Lizenz, das tatsächlich eingebettete VST3-SDK
  und wesentliche Drittkomponenten müssen versioniert und inventarisierbar
  sein.
- Dieser Vorschlag genehmigt **keine** Abhängigkeitseinbindung, Installation,
  Implementierung oder Verteilung. Das erfordert eine gesonderte Freigabe.

**Unbestätigte Versuchswerte:** N=2048, Hop=512 und
sqrt(periodische Hann) werden hier als zu untersuchende Versuchskonfiguration
behandelt, nicht als durch ADR 0001 bereits freigegebene DSP-Entscheidung.
Bei 44,1/48 kHz entsprechen sie einer Fensterdauer von ca. 46,44/42,67 ms,
einem Hop von ca. 11,61/10,67 ms und einer Binbreite von ca. 21,53/23,44 Hz.
Die tatsächliche Plugin-Latenz folgt nicht automatisch aus N; sie ist am
verarbeiteten Impuls zu messen und dem Host entsprechend zu melden.

## Optionen

Die Echtzeitvorgabe ist ein **Ausschlusskriterium**, kein gegen CPU oder
Bequemlichkeit aufzuwiegender Punktwert. Unter den Optionen, die sie
erfüllen, werden ein früher prüfbarer neutraler Pfad und geringe
Wartungs-/Wechselkosten priorisiert. Es wurden keine ungewichteten
Punktzahlen vergeben.

| Kriterium | A: JUCE `dsp::FFT` (Standard-Fallback) | B: PFFFT 1.3.0 | C: eigener Radix-2-FFT-Kern |
| --- | --- | --- | --- |
| Muss-Anforderungen | **Verletzt:** Der Fallback nimmt in `perform()` einen `SpinLock`. Bei N=2048 bleibt der Real-Transform-Scratch mit 16.400 Byte unter JUCEs 256-KiB-Schwelle und liegt auf dem Stack, aber der Lock bleibt. FFT-Objekt und Tabellen müssten außerhalb des Callbacks angelegt werden. | **Erfüllbar unter Bedingungen:** Setup und alle ausgerichteten Ein-/Ausgabe- und Work-Puffer vorab erzeugen; bei jedem Transform ein eigenes, nicht-null Work-Array übergeben. Source-Review und Upstream-Dokumentation beschreiben keinen Lock und keine Heap-Allokation im Transformpfad. Laufzeitmessung bleibt nötig. | **Erfüllbar, aber unbewiesen:** Lock-/Allokationsfreiheit ließe sich von Anfang an gestalten; ohne Code und Tests ist das keine Evidenz. |
| Audioqualität / Artefaktrisiko | Fensterung und WOLA können transparent sein, wenn Normalisierung und Randbehandlung korrekt sind; FFT allein beweist das nicht. | Gleiche grundsätzliche STFT-Risiken. PFFFT liefert Real-Spektren in eigener, ungeordneter Datenanordnung; falsches Reordering oder Skalierung erhöht das Rekonstruktionsrisiko. | Gleiches STFT-Risiko plus Implementierungs-/Numerikfehler eines neuen FFT-Kerns. |
| Latenz | FFT-Aufruf bestimmt die STFT-Pufferlatenz nicht allein; Fenster-Ausrichtung, Frame-Scheduling und Overlap-Puffer sind entscheidend. | Gleiche Latenzcharakteristik bei gleicher STFT-Architektur; kein belegter Latenzvorteil. | Gleiche Architektur; Implementierung kann Scheduling und Speicherlayout frei gestalten, aber noch nicht belegt. |
| CPU / Speicher | JUCE bezeichnet den Fallback als einfache, nicht auf Geschwindigkeit getunte Implementierung. Temporärer Stack-Scratch und SpinLock; Messung fehlt. | SIMD-Implementierungen für x86; Zustand und Work-Speicher sind planbar. CPU-Last für den konkreten Stereo-/Hostfall ist nicht gemessen. | Keine zusätzliche Library; Laufzeit, SIMD-Optimierung und Speicherbedarf müssten selbst entwickelt und gemessen werden. |
| Echtzeit-Tauglichkeit | **Nicht akzeptabel als Standardkonfiguration:** Lock im Callback. `JUCE_DSP_USE_SHARED_FFTW` und `JUCE_DSP_USE_STATIC_FFTW` sind im JUCE-Header standardmäßig 0; daher ist der Fallback unter Windows und Linux der Ausgangspfad, sofern das Projekt nicht anders konfiguriert wird. | **Voraussichtlich passend:** PFFFT dokumentiert allocation-freie Transformfunktionen und Setup-Sharing. Für den Callback müssen Work- und Datenpuffer ausgerichtet, vorallokiert und getrennt je gleichzeitigem Transform sein. Keine Echtzeitgarantie ohne Prüfung und Laufzeittest. | Kann RT-tauglich werden, aber kein bestehender Code-/Reviewnachweis. |
| Testbarkeit | Gut: ein Offline-Null-/Impulstest kann die FFT und WOLA prüfen; der Lock ist zusätzlich statisch und dynamisch nachzuweisen. | Gut: derselbe DSP-Kern kann Offline und im Host genutzt werden; PFFFT-Layout, Skalierung, Nullrest und Callback-Allokationen lassen sich getrennt prüfen. | Gut testbar, sobald vorhanden; höherer Testumfang für die FFT selbst. |
| Abhängigkeiten / Wechselkosten / Reversibilität | Keine zusätzliche FFT-Library; geringe Einbindungskosten, aber harte RT-Abweichung oder späterer Backendwechsel. FFT hinter schmaler DSP-Schnittstelle kapseln. | Eine zusätzliche, portable C-FFT-Abhängigkeit, Lizenzhinweis und eigener Adapter für Datenlayout/Skalierung. Mittlere, durch DSP-Schnittstelle begrenzbare Wechselkosten. | Keine Drittbibliothek, aber größte Entwicklungs-, Review- und Wartungskosten. Wechsel zu einer geprüften FFT bleibt möglich, wenn die Schnittstelle abstrahiert wird. |
| Evidenz / offene Punkte | JUCE 9.0.5 Quelltext und Header; die im Standardpfad auftretende tatsächliche Host-Latenz und Laufzeit sind noch nicht gemessen. | Release v1.3.0 und Dokumentation; keine Spectral-Decay-Performance-, Neutralitäts- oder Hostmessung. Die Zuordnung des Textlizenzes zum SPDX-Identifier muss im Lizenzinventar bestätigt werden. | Nur Entwurfsoption; kein Code und keine Benchmarks. |

### Abwägung und Empfehlung

**Vorgeschlagene Kombination:** JUCE **9.0.5**, Tag `9.0.5`, voller
Commit-SHA
`d60b7a78030661574a615b586d9325ba7a7fa7dd`, plus PFFFT **v1.3.0**,
voller Commit-SHA
`179a48d486cbee8ec5416e5c163ad575c2efe1f1`. PFFFT ist ein Vorschlag, keine
Einbindung.

JUCE 9.0.5 ist ein tatsächlicher stabiler Release-Tag vom 2026-11-02. Der
taggebundene [JUCE SPDX-SBOM](https://github.com/juce-framework/JUCE/blob/9.0.5/JUCE.spdx.json)
führt für die JUCE-Module `AGPL-3.0-only OR LicenseRef-JUCE-Commercial`
auf, und für das tatsächlich vendorte VST3-SDK Version **3.8.0**, Lizenz
**MIT**. Die passende
[JUCE-Lizenzdatei](https://github.com/juce-framework/JUCE/blob/9.0.5/LICENSE.md)
beschreibt denselben AGPLv3-/kommerziellen Dual-Lizenzpfad. Die Kennung ist
**AGPL-3.0-only**, nicht `AGPL-3.0-or-later`; das ist für die im ADR 0002
vorgesehene eigene Lizenz `AGPL-3.0-or-later` gesondert rechtlich zu prüfen.

Der JUCE-9.0.5-FFT-Fallback ist auf Windows und Linux für den hier
vorgeschlagenen Callback nicht zulässig: Der Standard setzt FFTW-Optionen
nicht, und der Fallback sperrt seinen `processLock` in `perform()`.
N=2048 vermeidet zwar dessen Heap-Scratch-Ausweichpfad, aber nicht den Lock.
PFFFT ist gegen den Eigenbau abzuwägen, weil es eine ausgereifte, portable
Version mit MSVC-/Linux-Testabdeckung bietet, während ein selbst entwickelter
Kern die Abhängigkeitszahl senkt, aber neue numerische und Wartungsrisiken
schafft. Der PFFFT-Adapter muss wegen des PFFFT-Spektrallayouts und der
Normalisierung eigenständig gegen Nulltest und Referenzsignale geprüft
werden. CPU-Vorteile werden nicht behauptet.

Für sqrt(periodische Hann) mit Hop=N/4 ergibt die Analyse-/Synthesefenster-
Produktfolge rechnerisch eine konstante Überlappungssumme von 2 im Inneren
des Signals. Das legt eine entsprechende WOLA-Normierung nahe; Ein- und
Ausschwingränder sowie FFT-Rundungsrest bleiben zu messen. Das ist keine
Abnahmegrenze und kein Nachweis eines transparenten Pfads.

## Lizenz- und Abhängigkeitsinventar (Recherche, kein Rechtsgutachten)

| Bestandteil | Identifikator / Version | Bedeutung für den eingeschränkten VST3-Prototyp |
| --- | --- | --- |
| JUCE-Frameworkmodule | SPDX `AGPL-3.0-only OR LicenseRef-JUCE-Commercial` | AGPL-Pfad entspricht in der gewählten Version **nicht** `or-later`. Proprietäre/kommerzielle Alternative ist nicht entschieden. |
| Eingebettetes Steinberg VST3-SDK | **3.8.0**, **MIT**; Copyright-Hinweis Steinberg 2025 | Im JUCE-9.0.5-SBOM explizit mit Quellpfad belegt. MIT-Notice bei Verteilung erhalten. Marken-/Namensnutzung separat prüfen. |
| PFFFT | **v1.3.0**, Lizenztext BSD-Stil; vorgeschlagene SPDX-Zuordnung **BSD-3-Clause** | Upstream-Text verlangt Copyright-/Bedingungs-/Haftungshinweise in Source und Binärverteilung sowie untersagt Endorsement ohne Erlaubnis. Das Upstream-Repository kennzeichnet dies als „BSD-like“, nicht mit einer expliziten SPDX-Zeile; Mapping noch im Lizenzinventar bestätigen. |
| JUCE-Core-Zlib | **1.3.2**, SPDX `Zlib` | In JUCE vendort; zugehörigen Lizenztext und Hinweise bei verwendeten Modulen nach dem SBOM prüfen. |
| JUCE-Grafik/Text-Bausteine | HarfBuzz **15.0.0** (`MIT-Modern-Variant`), SheenBidi **2.9.0** (`Apache-2.0`), LunaSVG **3.5.0** (`MIT`), PlutoVG **1.3.2** (`MIT`), libpng **1.6.58** (`libpng-2.0`), jpeglib **10.0** (`IJG`), libwebp **1.6.0** (`BSD-3-Clause`) | Relevante vendorte Einträge für grafische JUCE-Module; welche Quellen in einem konkreten Minimalziel tatsächlich kompiliert/mitverteilt werden, hängt von den ausgewählten Modulen und Optionen ab. Das SBOM des exakten Tags ist maßgeblich. |
| Nicht für VST3-only vorgesehene JUCE-SDKs | ASIO **2.3** (`LicenseRef-Steinberg-ASIO OR GPL-3.0-only`), AAX **2.9** (`LicenseRef-Avid-AAX OR GPL-3.0-only`), LV2 **1.18.0** (`ISC`) samt LV2-Komponenten | In JUCE vorhanden, aber nicht als benötigte Formate freigeben. Nicht `VST`, `AAX`, LV2-Hosting oder Geräte-/Audio-Utils hinzufügen, solange sie nicht ausdrücklich gebraucht werden. |

**Minimale Abhängigkeitsrichtung:** C++-DSP-Kern einschließlich STFT und
PFFFT von `AudioProcessor`, Pluginformat und optionaler GUI trennen.
JUCEs VST3-Pluginhülle auf **VST3** beschränken, unnötige Beispiel-,
Extra-, Host-, Format- und Geräteziele weglassen und nicht blind die
Beispielverknüpfung `juce_audio_utils` übernehmen. Die genaue minimale
JUCE-Modulmenge und die vom CMake-Pluginziel tatsächlich kompilierten
Transitiva sind in einem freigegebenen Prototyp-Build zu verifizieren.
Insbesondere hängt `juce_dsp` laut JUCE-Moduldeklaration von
`juce_audio_formats` ab; es wird für die vorgeschlagene externe FFT nicht
benötigt.

## Buildwerkzeuge (keine Plugin-Laufzeitabhängigkeiten)

- **CMake 3.31.11**: tatsächlicher Kitware-Release, deutlich über JUCEs
  Mindestanforderung **3.22**; bewusste konservative Wahl gegenüber dem
  neueren CMake-4-Zweig für ein VS-2022-Projekt.
- **Visual Studio 2022 17.14.44**, Current Channel, **MSVC v143 / 14.44**,
  Workload „Desktop development with C++“; x64-Generator/Architektur.
- **Windows SDK 10.0.26100.9457**: konkreter Windows-11-SDK-Release aus
  Microsofts Downloadtabelle. SDK-Version ist Buildumgebung, nicht
  Laufzeit-/Mindest-Windows-Version des Plugins.

CMake, MSVC und das Windows SDK sind Buildtools bzw. Plattform-SDKs und
werden nicht als JUCE-/DSP-Laufzeitbibliotheken in das Plugin aufgenommen.
Für Linux kann derselbe DSP-Kern separat mit einem unterstützten C++17-
Compiler getestet werden; ein Linux-Lauf ersetzt den Windows-/Ableton-
Nachweis nicht.

## Vorgeschlagener Validierungsschritt

Kleinster aussagekräftiger Schritt nach gesonderter Nutzerfreigabe:
ein schmaler **DSP-/FFT-Harness ohne Plugin und ohne GUI**, der genau eine
reale Spur durch N=2048, Hop=512 und sqrt(periodische Hann) verarbeitet.
Setup, Fenster, FFT-Puffer und Work-Array vorab erzeugen; einen Impuls und
deterministisches Rauschen sowohl als Offline-Referenz als auch mit
Callback-Blockgrößen 1, 64, 512 und einer ungeraden Größe durchleiten.
Protokollieren: Nullrest/RMS/Peak, Impulsposition, endliche Samples,
Allokations-/Lock-Instrumentierung und Zeit pro Block im Windows-x64-
Releasebuild. Anschließend denselben DSP-Kern unter Linux laufen lassen.
Messwerte berichten; verbindliche CPU- und Neutralitätsschwellen bleiben
offen, wie in Spec und QS-05 markiert. Erst danach folgt der separate
VST3-/Ableton-Live-12-Hostnachweis.

## Entscheidung

**Vorgeschlagen, nicht freigegeben:** JUCE 9.0.5 an obigem Commit und
PFFFT v1.3.0 an obigem Commit für den neutralen Untersuchungspfad prüfen.
PFFFT wird gegenüber JUCEs Standard-FFT empfohlen, weil die festgehaltene
Echtzeitregel einen Callback-Lock ausschließt und PFFFT den kleinsten
konkreten, portablen Ersatz mit dokumentierten, vorbereitbaren Puffern
darstellt. JUCE bleibt auf die Plugin-/Host-Hülle beschränkt; der
plattformunabhängige DSP-Kern bleibt separat.

Die **Produktverantwortung/Nutzerin oder der Nutzer** muss vor jedem Bezug,
Einbinden oder Implementieren ausdrücklich entscheiden über:

1. JUCE 9.0.5 und die Konsequenz `AGPL-3.0-only` im Gegensatz zur eigenen
   vorgesehenen `AGPL-3.0-or-later`-Kennung;
2. den zusätzlichen PFFFT-Baustein und sein Lizenz-/Notice-Inventar;
3. die Konfiguration N=2048/Hop=512/sqrt(periodische Hann) als Versuchswert
   oder als verbindliche Prototypkonfiguration;
4. ob weitere rechtliche Prüfung (einschließlich späterer proprietärer
   Distribution und VST3-Markenregeln) vor einem Build erforderlich ist.

## Konsequenzen und offene Punkte

- **Positiv:** echte, gepinnte Release- und SDK-Versionen statt
  `master`/`latest`; Echtzeitabweichung des JUCE-Standardfallbacks ist vor
  Implementierung erkannt; DSP bleibt plattformunabhängig testbar.
- **Negativ / Folgekosten:** eine zusätzliche FFT-Abhängigkeit und ein
  Reordering-/Skalierungsadapter; Drittanbieterhinweise müssen für die
  tatsächlich verwendeten Module verifiziert werden.
- **Frühe Warnsignale:** inkorrekter FFT-Binspeicher, falsche inverse
  Normierung, nicht konstanter WOLA-Gain, Nullrest abhängig von
  Blockgrenzen, Allokation/Lock im Callback, nicht tragbare CPU oder
  abweichende Hostlatenz.
- **Offen / nicht belegt:** keine Messung von CPU, Neutralität, Latenz,
  hörbaren Artefakten oder PFFFT-Callbackverhalten im konkreten Plugin;
  kein Windows-/Ableton-/Linux-Build; keine rechtliche Bewertung der
  Lizenzkombination oder Vertriebssituation. JUCEs mitgelieferte
  VST3-SDK-README enthält ältere Hinweise auf eine zu unterzeichnende
  Vereinbarung, während Steinbergs aktuelle 3.8-Lizenzseite MIT ohne
  Mitgliedschaft/Unterschrift beschreibt. Für tatsächliche Pflichten und
  Markenverwendung bleibt eine menschliche Prüfung nötig.

### Quellen (Versionen am 2026-11-15 geprüft)

- JUCE [9.0.5 Release und Commit](https://github.com/juce-framework/JUCE/releases/tag/9.0.5);
  [taggebundener `LICENSE.md`](https://github.com/juce-framework/JUCE/blob/9.0.5/LICENSE.md);
  [taggebundener SPDX-SBOM](https://github.com/juce-framework/JUCE/blob/9.0.5/JUCE.spdx.json).
- JUCE [FFT-Implementierung](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_dsp/frequency/juce_FFT.cpp),
  [FFT-API](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_dsp/frequency/juce_FFT.h)
  und [DSP-Konfigurationsheader](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_dsp/juce_dsp.h).
- JUCE [CMake API, Mindestversion und Modulhinweise](https://github.com/juce-framework/JUCE/blob/9.0.5/docs/CMake%20API.md).
- JUCE vendortes [VST3-SDK-README](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_audio_processors_headless/format_types/VST3_SDK/README.md),
  [MIT-Lizenzdatei](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt)
  und [Upstream-Hinweis](https://github.com/juce-framework/JUCE/blob/9.0.5/modules/juce_audio_processors_headless/format_types/VST3_SDK/JUCE_UPSTREAM.txt);
  Steinbergs [Version 3.8.0](https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Version+3.8.0.html)
  und [VST3-Lizenzseite](https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/VST3+License.html).
- PFFFT [v1.3.0 Release/Commit](https://github.com/marton78/pffft/releases/tag/v1.3.0),
  [README](https://github.com/marton78/pffft/blob/v1.3.0/README.md),
  [API und Puffervertrag](https://github.com/marton78/pffft/blob/v1.3.0/include/pffft/pffft.h)
  sowie [Lizenztext](https://github.com/marton78/pffft/blob/v1.3.0/LICENSE.txt).
- Kitware [CMake 3.31.11 Release](https://www.kitware.com/cmake-3-31-11-available-for-download/);
  Microsoft [Visual Studio 2022 Release-History](https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-history)
  und [Windows-SDK-Downloads](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads).

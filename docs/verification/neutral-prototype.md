# Neutraler Prototyp: Pruefprotokoll

Stand: 2026-11-15. **Historischer Nachweis des neutralen Zwischenstands:
offline und Linux-VST3 validiert; Windows/Ableton offen.** Dieses Protokoll
beschreibt nicht den inzwischen erweiterten Gesamtstand. Der spätere
Verlängerungszweig ist im [separaten Tail-Prüfbericht](tail-prototype.md)
dokumentiert. Beide Berichte lassen offene numerische Produktgrenzen
unbewertet.

## Auftrag und Freigaben

Die Produktverantwortung hat im Interview freigegeben:
JUCE 9.0.5 und PFFFT v1.3.0 nach Lizenzpruefung, neutrale STFT mit
N=2048/H=512/sqrt(periodische Hann), Mono/Stereo, 0-Sample-No-op,
Reset/Reprepare und Messwerte vor verbindlichen numerischen Grenzen.
Anschliessend wurde die Umsetzung ausdruecklich beauftragt.
Zwei GPT-Luna-Subagenten implementierten DSP und Offline-Tests.

## Implementierter Signalweg

`NeutralAudioProcessor::processBlock` setzt `ScopedNoDenormals` und ruft
denselben `NeutralStft`-Kern wie der Offline-Test auf. Pro Kanal:
Eingangsring -> Analysefenster -> geordnete PFFFT-Vorwaerts-FFT ->
unveraendertes Spektrum -> inverse FFT -> Synthesefenster -> Overlap-Add.
Keine direkte verzögerte Kopie als Neutralpfad.

Es werden bereits nach dem ersten Hop linksseitig mit Nullen ergaenzte
Frames verarbeitet. Dadurch erhalten auch die ersten Eingangssamples alle
vier Fensterbeitraege. Die Summe der vier periodischen Hann-Produkte ist
rechnerisch 2; zusammen mit PFFFTs unnormierter inverser FFT folgt
der Framefaktor `1/(2N)`.

Ein nach Eingangssample `n` bearbeitetes Frame beginnt am Ausgang bei
`n+1`. Seine Frameposition `i` erscheint dort bei `n+1+i`; die zugehoerige
Eingangsposition war `n+1-N+i`. **Aus diesem Scheduling abgeleitete und
vom Processor gemeldete Latenz: 2048 Samples**, also rechnerisch
46,4399 ms bei 44,1 kHz bzw. 42,6667 ms bei 48 kHz.
**Bestaetigt:** 258 eindeutige Offline-Impulsmessungen sowie alle
24 Kanal-/Layout-/Rate-/Block-Kombinationen je Processor- und VST3-Hostlauf
ergaben genau die gemeldeten 2048 Samples. Der Modultest laedt die echte
Linux-VST3-Binaerdatei, nicht nur den direkt instanziierten Processor.

Es gibt keinen beabsichtigten Decay-Tail. Die Host-Tailmeldung reserviert
konservativ Latenz plus ein Rekonstruktionsfenster fuer moegliche
FFT-Rundungsreste. Die Tests spuelen laenger nach; diese Meldung ist
ebenfalls noch nicht im Host bestaetigt.

## Reproduzierbare Offline-Matrix

Ziel: `spectral_decay_offline_tests --report <pfad.csv>`, durch CTest
automatisch mit dem Berichtspfad im Buildordner aufgerufen.

| Bedingung | Definition |
| --- | --- |
| Sampleraten | 44100 und 48000 Hz |
| Hostbloecke | 1, 64, 512 und 257; letzter Teilblock in tatsaechlicher Laenge |
| Layouts | Mono sowie Stereo; Auswertung je Ausgangskanal |
| Zustand | Neue Vorbereitung und Reset vor unabhaengigen Signalversuchen |
| Flush | Gemeldete Latenz plus 4N Nullsamples, nicht vom Vergleich abgeschnitten |
| Nullreferenz | `y[k] - x[k-L_report]`, Eingang ausserhalb seiner Laenge null; keine frei optimierte Verschiebung |
| Fenster | Gesamtausgabe, erste 4N und letzte 4N; genaue halb offene Grenzen und Laengen in CSV |
| Blockvergleich | Gleiche Signale gegen Block 1, anhand der gemeldeten Latenzen ausgerichtet |

Signale:

- Stille 1 s; Einheitsimpulse an Start, Block-/Hop-/Fenstergrenzen sowie
  am letzten Sample eines 3N langen Eingangs; fuer Stereo links/rechts getrennt.
- Acht Impulse, abwechselnd +0,25/−0,25, im Abstand von 250 ms, insgesamt 2 s.
- Logarithmischer Sweep 20 Hz bis 20 kHz, 10 s, Amplitude 0,25,
  50-ms-Randfenster.
- Rauschburst 0,5 s plus 0,5 s Stille, Amplitude 0,1. xorshift32 mit
  unsigned 32-bit Wraparound und Schritten `x ^= x<<13; x ^= x>>17; x ^= x<<5`,
  Skalierung auf `0.1*(2*x/2^32-1)`, Seeds `0x6D2B79F5` und `0x1B873593`.
- 1-kHz-Sinus, 1 s, Spitzenpegel −0,1 dBFS.
- Je 1 s Sinus bei 31,25/62,5/125/250/500/1000/2000/4000/8000/12000/
  16000/20000 Hz, Amplitude 0,25; Auswertung jeweils letzte 0,5 s.
- Stereo zusaetzlich L=R, unabhaengige Seeds und einseitiges Rauschen.

Ton-Gain: RMS-Verhaeltnis im ausgerichteten Messfenster.
Frequenzschaetzung: lineare Least-Squares-Regression aller interpolierten
positiven Nulldurchgaenge gegen ihren Index mit freiem Phasen-Achsenabschnitt.
Input und Output verwenden dasselbe latenz-ausgerichtete Fenster und
denselben Schaetzer. Nominale Frequenz, Input-/Output-Schaetzung und
Output-minus-Input-Differenz werden getrennt berichtet; das Verfahren
ist keine perfekte absolute Frequenzreferenz.
CSV protokolliert Null-Peak/RMS, Ausgangspegel, Ton-Gain und Frequenzen,
Stereo-Korrelation, gemeldete/gemessene Impulslatenz und Blockdifferenzen.
Lineare FS-Werte und dB-Werte sind getrennt bezeichnet; exakte Null entspricht
−inf dBFS und ist kein nichtendliches Audio-Sample.

Harte Strukturpruefungen werden getrennt von `MEASUREMENT_ONLY`-Zeilen gezaehlt:
Endlichkeit, eindeutiger Impulspeak mit exakt gemeldeter Verzoegerung,
unveraenderte Ausgabe nach Reset/0-Sample-Bloecken/erneuter Vorbereitung
und echte Sampleratenwechsel. Einseitige Anregung darf den anderen
unabhaengigen Kanal nicht fuellen. Es gibt keine erfundene Nulltest-,
Frequenz-, Stille- oder Blockdifferenztoleranz. CTest-Erfolg ist nur ein
Ergebnis der definierten Strukturchecks, keine vollstaendige AK1–AK5-Abnahme.

## Tatsaechlich ausgefuehrte Checks

Umgebung: Ubuntu 24.04, Linux x86-64, GCC/G++ 13.3.0, CMake 3.28.3,
Release, PFFFT-Single-Precision/SSE. PowerShell 7.7.0 fuer die Skriptpruefung.
Alle Quellen lagen noch im nicht committeden Arbeitsbaum.

| Check | Ergebnis |
| --- | --- |
| GitHub-Tag/Commit-Aufloesung und Original-Lizenz-/SBOM-Lektuere | ausgefuehrt; gepinnte Quellen/Hashes im Buildmanifest |
| Fruehere Konfigurations-/Export-Sperren | Nach neuem menschlichem Wiederholungsauftrag erneut versucht; die nachfolgenden Schritte wurden zugelassen |
| `cmake --preset offline` / `cmake --build --preset offline` / `ctest --preset offline` | erfolgreich; 1/1 Testprogramm |
| `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON` | erfolgreich; alle Tests aktiviert |
| Erster vollstaendiger Plugin-Build | Fehlender Header `Xrandr.h`; fehlende Xrandr-/Xinerama-/Xcursor-/Xi-Entwicklungsheader gezielt installiert, kein Abschalten von Tests oder Features |
| `cmake --build build --parallel 2` | erfolgreich; Linux-VST3, direkter Processor-Test und VST3-Testhost gelinkt |
| `ctest --test-dir build --output-on-failure` | 3/3 erfolgreich: `neutral_offline`, `neutral_processor`, `neutral_vst3_host`, auch nach den Zustandsproben mit aktivem Overlap |
| Umbenanntes Bundle mit passend umbenannter `.so` im echten Host | erfolgreich; 24 Kombinationen erneut geprueft, nur Linux-Nachweis |
| PFFFT-C-Quellen mit `cc -std=c99 -fsyntax-only` | erfolgreich |
| DSP und Offline-Harness mit `c++ -std=c++17 -Wall -Wextra -Wpedantic -fsyntax-only` | erfolgreich |
| Processor, Processor-Tests und VST3-Hosttest mit `-fsyntax-only` und JUCE-Headern | erfolgreich |
| Preset-JSON und `cmake --list-presets` | erfolgreich; keine Konfiguration dabei |
| PowerShell-Parser, echte CTest-JSON-Auswertung und Linux-Export-Abweisung | erfolgreich; keine Windows-Ausfuehrung des Exports |
| Workflow-YAML, manueller Trigger und Repository-Leserechte | statisch geprueft; kein GitHub-Lauf gestartet |
| Windows x86-64, Ableton Live 12, Hoertest | **nicht ausgefuehrt** |

Eine Syntaxpruefung erkennt weder fehlende Link-Symbole noch DSP-/Hostfehler
und ersetzt keinen Build oder Audio-Test. Die nachfolgenden Werte stammen
aus den jetzt tatsaechlich ausgefuehrten Linux-Laeufen, nicht aus den
vorangegangenen Syntaxpruefungen.

## Messwerte

Rohdaten: `build/offline-metrics.csv`, alternativ durch das DSP-Preset
reproduzierbar unter `out/offline/offline-metrics.csv`.
5214 CSV-Zeilen: **1438 bestandene harte Checks, 0 Fehler** und
**3776 reine Messzeilen**. Jede Rohdatenzeile nennt Bedingungen, Kanal,
Fenster, Laengen und gemeldete Latenz.

| Groesse | Beobachtung | Bedingung / Bewertung |
| --- | --- | --- |
| Impulslatenz | gemeldet = gemessen = 2048 Samples | Alle 258 Offline-Impulse eindeutig; exakter Strukturcheck bestanden |
| Nullrest-Peak, groesster Wert | 4,172325134e-7 FS = −127,5924371 dBFS | 44,1 kHz, Block 1, Mono, −0,1-dBFS-Sinus; **nur gemessen**, Toleranz offen |
| Nullrest-RMS, groesster Wert | 8,185837300e-8 FS = −141,7387378 dBFS | 48 kHz, Block 1, Mono, −0,1-dBFS-Sinus; **nur gemessen**, Toleranz offen |
| Blockgroessendifferenz | Peak und RMS 0 FS | Alle 426 Vergleichszeilen gegen Block 1; **nur gemessen**, keine erfundene Abnahmegrenze |
| Stille | Ausgang und Nullrest 0 FS | Alle Raten/Bloecke/Layouts im Stilleversuch; **nur gemessen**, allgemeine Stille-Grenze offen |
| Inaktiver Stereokanal | exakt 0 FS | 188 harte Kanaltrennungschecks bestanden |
| Ton-Gain | −6,222282255e-7 bis +1,691648529e-7 dB | Letzte 0,5 s je Ton, alle Bedingungen; **nur gemessen** |
| Frequenzschaetzung, groesste absolute Output-minus-Input-Differenz | 8,131246432e-8 Hz | 20 kHz bei 44,1 kHz; **nur gemessen**, keine Zusage dieser absoluten Messgenauigkeit |
| Absoluter Frequenzschaetzer-Bias gegen Nominalfrequenz | etwa +0,00026 Hz | Input und Output bei 20 kHz/44,1 kHz; kein nachgewiesener Plugin-Frequenzfehler |
| Endlichkeit | alle geprueften Ausgangssamples endlich | 638 harte kanalweise Checks bestanden |

Reset nach lebendem 2688-Sample-Zustand, 0-Sample-Aufrufe an Start/Mitte/Ende,
Reprepare und echte 44,1↔48-kHz-Wechsel gegen frische Instanzen sind Teil
der Strukturchecks. Die Zustandsproben sind laenger als FFT und Hop zusammen,
damit nicht nur der Eingangsring, sondern auch echte FFT-/Overlap-Zustaende
und ausgegebene Audiosamples beteiligt sind. Nachfolgesignal und Flush
machen Aenderungen am Ende eines 0-Blocks bzw. Ratewechsels beobachtbar.
CPU-Auslastung/Worst-Case-Callbackzeit und
Allokationen/Locks sind durch diese Laufzeiten **nicht** qualifiziert.

## Echtzeit-Quellreview und Restrisiken

Eigener Callbackpfad: `processBlock` -> `NeutralStft::process` ->
`Impl::processFrame` -> PFFFT Transform mit explizitem Work-Array.
FFT-Setup, Fenster, Ringe und SIMD-ausgerichtete Puffer werden in `prepare`
erzeugt, nicht im Callback. Gueltige Aufrufe enthalten nach Quellreview keine
eigenen Allokationen, Locks, I/O oder Throws. Ungueltige API-Aufrufe werden
durch dokumentierte Vorbedingungen/Debug-Assertions begrenzt; die
Vorbereitung validiert Rate und Layout mit Ausnahmen ausserhalb des Callbacks.
Keine Allokations-, Lock- oder Laufzeitinstrumentierung wurde ausgefuehrt.

**Bekannter Frameworkverstoss gegen einen vollstaendig lockfreien
Gesamtaudiopfad:** JUCE 9.0.5 nimmt in
`modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp:3777`
vor `processBlock` einen `ScopedLock(pluginInstance->getCallbackLock())`.
Damit ist das 0-Lock-Ziel von QS-05 fuer den gesamten VST3-Pfad **nicht
nachgewiesen und laut Quellfund nicht erfuellt**. Der eigene DSP-Pfad und
der Frameworkpfad sind nicht gleichzusetzen. Keine riskante ungepruefte
Vendor-Aenderung wurde vorgenommen.

JUCE stellt ausserdem einen formatbedingten Host-Bypass bereit. Das ist
keine hier entwickelte oder gepruefte Bypass-Funktion. Kein Schutz fuer
NaN/Inf am Eingang, kein Limiter und keine persistente Benutzerkonfiguration.
Rechte an Beitraegen, vollstaendige Notices am fertigen Windows-Artefakt
und Distribution bleiben gesondert zu klaeren.

## AK-Abgleich

| Kriterium | Stand |
| --- | --- |
| AK1 Neutralitaet | Gemessen; keine Abnahme ohne beschlossene Grenzen |
| AK2 Latenz | Offline und Linux-Host exakt bestaetigt; Windows noch offen |
| AK3 Blockgroessen | Im Versuch bitgleiche Ausgaben gemessen; formale Toleranz offen |
| AK4 Endlichkeit/Stille | Endlichkeit bestaetigt; Stille 0 FS gemessen, allgemeine Grenze offen |
| AK5 Windows/Host | Linux-Modul geladen/geprueft; Windows-/Ableton-Nachweis ausstehend |

Weiter: [Windows-Export/Download](export.md) und
[Windows-/Ableton-Anleitung](windows-ableton.md).
Kein Commit/Push, kein externer Build oder Windows-Workflow wurde hier gestartet.
Die vorgeschlagenen Dokumentationsdateien ADR 0003/QS-05 aus der
Recherchephase wurden hier nicht als durchgefuehrter Nachweis behandelt.

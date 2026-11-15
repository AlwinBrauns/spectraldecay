# 0005 – Zweiter Prototyp mit bipolarem Peak-Band

- **Status:** vorgeschlagen
- **Datum:** 2026-11-15
- **Entscheider:** Produktverantwortung; DSP und Test beraten
- **Bezug:** [Produktvision](../product/product-vision.md), [ADR 0001](0001-prototyp-architektur.md), [ADR 0003](0003-juce-fft-und-buildkette.md), [ADR 0004](0004-erster-verlaengerungsprototyp.md), [Qualitätsszenarien QS-06–QS-09](../quality/szenarien.md), [Spec Prototyp 2](../specs/prototyp-zwei-peak-band.md)

## Entscheidungsfrage und Kontext

Welcher reversible DSP- und Bedienungsansatz soll Prototyp 2 auf dem
vorhandenen STFT-Kern untersuchen, um frequenzselektives Verkürzen und
Verlängern mit bipolarer Decay-Steuerung und stufenlosem Tone/Noise-Morph zu
erproben, ohne zusätzliche Look-ahead-Latenz oder eine neue Abhängigkeit
vorauszusetzen?

### Befund aus Prototyp 1

**Belegt als Auftraggeberbericht:** Beim Windows-/Ableton-Hörtest am
2026-11-15 wurden folgende Bedienungs- und Klangprobleme berichtet:

1. Ein nur verlängernder, unipolarer Tail-Regler reicht nicht. Gewünscht ist
   `bandDecay` in `[-5, +5]`: 0 neutral, positive Werte länger, negative
   Werte kürzer.
2. Ein binärer Tonal/Noise-Schalter reicht nicht. Gewünscht ist ein
   stufenloses `toneNoise` in `[0, 1]`.
3. Zwei Low/High-Frequenzschieber sind unintuitiv. Gewünscht ist ein
   EQ-artiges Peak-Band mit `bandFreqHz` in `40..16000 Hz` und
   `bandQ` in `0.25..12.0`.
4. „Noise“ klang „hallig“; die berichtete P1-Umsetzung verwendete
   unkorrelierte Gleichverteilungs-Zufallsphase und eine starre 400-ms-
   Hüllkurve auf allen Bins im Band.
5. „Tone“ klang „granular“; berichtet wurden unabhängige binweise
   Phasenvokoder-Phasen ohne Peak-Phase-Locking benachbarter Bins sowie
   hopweise Aktivierungssprünge.

Hörtestprotokoll, Material, Einstellungen und Messungen sind nicht verfügbar.
Diese Beobachtungen sind deshalb qualitative Evidenz für die
Prototypausrichtung, keine formale Windows-/Ableton-Abnahme. Der vorhandene
Quellstand belegt N=2048, H=512 sowie unipolaren Tail, binären Modus und
Low/High-Grenzen; er belegt nicht die Klangqualität der gewünschten
Änderungen.

### Evidenzstatus

- **Belegt:** Der archivierte P1-Quellstand verwendet N=2048/H=512 und hat
  einen unipolaren Tail-Regler, einen binären Tone/Noise-Modus und Low/High-
  Bandgrenzen. Die fünf Windows-/Ableton-Beobachtungen sind als Bericht des
  Auftraggebers vom 2026-11-15 belegt, nicht durch ein hier vorliegendes
  Hörtestprotokoll.
- **Annahme:** Ein einzelnes Peak-Band mit bipolarer zeitlicher Steuerung und
  einem stetigen Tone/Noise-Morph ist ein geeigneter, reversibler Versuch,
  um diese Bedienungsprobleme zu untersuchen. Peak-Phase-Locking,
  transientenschonende Verkürzung und korrelierte Noise-Modulation könnten
  die gehörten Artefakte mindern; das ist nicht nachgewiesen.
- **Offen:** Formale Host-/Messreihen, Versionen und Testbedingungen,
  Q-zu-Breite-Abbildung, Klang-/CPU-/Echtzeitergebnis sowie sämtliche
  numerischen Abnahmeschwellen und die spätere Produktentscheidung.
- **Prototyp-2-Umsetzung:** Option B ist im Hauptbaum implementiert und
  unter Linux technisch verifiziert; Messwerte, Testumfang und Grenzen sind
  im [Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md)
  dokumentiert. Windows-/Ableton-Hörtest und numerische
  Abnahmetoleranzen stehen aus. Der ADR-Status bleibt daher bewusst
  **vorgeschlagen**; die technische Umsetzung ist keine Produktentscheidung.

### Muss-Anforderungen für den Versuch

- Im vorhandenen WOLA/STFT-Kern mit `N=2048, H=512` untersuchen; keine
  zusätzliche FFT-Größe und keine zusätzliche Look-ahead-Frame als
  Voraussetzung.
- `bandDecay` ist bipolar, `bandDecay == 0` neutral, positive Werte
  verlängern und negative Werte verkürzen. Der Nullpunkt muss den bestehenden
  neutralen WOLA-Pfad exakt unverändert lassen; angestrebt ist bit- bzw.
  gleitkommagleiche Ausgabe gegenüber diesem Pfad.
- `toneNoise` ist stetig in `[0,1]`; `0` tonal, `1` rauschhaft.
- Ein Peak-Band wird durch `bandFreqHz` und `bandQ` festgelegt; seine Wirkung
  fällt außerhalb der relevanten Bandbreite stetig ab.
- Verkürzung verändert den zeitlichen Ausklang, nicht nur den stationären
  Pegel. Verlängerung wird auf die vorläufige Zwei-Sekunden-Aktivitätsgrenze
  und das vorhandene Sample-Peak-Ceiling begrenzt; deren Produktdefinition
  und Messgrenzen bleiben offen.
- Echtzeitpfad: Speicher und Zustände vorab anlegen; keine Allokation,
  Sperre, I/O oder Logging im DSP-Callback. GUI greift nicht auf DSP-Puffer
  zu. Parameterübergabe und Glättung müssen nichtblockierend sein.
- Der Versuch ist umkehrbar und darf keine neue Abhängigkeit voraussetzen.
  Produktentscheidung und numerische Abnahmeschwellen bleiben offen.

## Optionen

Muss-Anforderungen sind Ausschlusskriterien. Innerhalb der noch
prototypischen Optionen werden zuerst das größte Entwicklungsrisiko
(Verlängerungsartefakte), der Erhalt des neutralen Pfads und die Möglichkeit
gewichtet, den Ansatz mit geringem Wechselaufwand zu verwerfen. Es werden
keine ungewichteten Punktzahlen vergeben.

| Kriterium | A: P1 minimal ändern | B: hybrides bipolares Peak-Band | C: vollständige Zerlegung / andere Engine |
| --- | --- | --- | --- |
| Muss-Anforderungen | Glocke und UI-Werte wären machbar. Eine statische Dämpfung für negative Werte erfüllt aber die zeitliche Verkürzung nicht; Noise-/Tone-Probleme bleiben. | Kann alle Muss-Anforderungen als Versuch erfüllen, sofern Nullpunkt und Echtzeitpfad explizit geprüft werden. | Könnte Anforderungen erfüllen, erfordert aber einen wesentlich größeren Versuch und verändert die Engine-Grundlage. |
| Audioqualität / Artefaktrisiko | Glockengewicht adressiert nur Bandform. Lineares Mischen der bestehenden Zweige lässt „hallig“ und „granular“ ungelöst. Statische Dämpfung färbt den Pegel, statt den Decay zu verkürzen. | Transientenschonende Hüllkurvendämpfung und Peak-Phase-Locking adressieren die berichteten Ursachen. Frequenzabhängige Noise-Dämpfung und korrelierte Modulation sind Hypothesen; metallische, stationäre oder phasenbedingte Artefakte bleiben möglich. | Zerlegung kann Materialklassen gezielter behandeln, bringt jedoch neue Übergangs-, Phasen- und Rekonstruktionsrisiken. Qualität nicht belegt. |
| Latenz | Geringe Änderung; keine zusätzliche Look-ahead-Latenz vorgesehen. | Vergangenheitsbasierte Tracker und Synthese im bestehenden WOLA-Raster sollen keine zusätzliche Look-ahead-Latenz benötigen. Hostlatenz und hörbare Reaktionszeit dennoch messen. | Multiresolution-STFT bzw. Filterbank kann andere Latenz-/Reaktionszeit-Kompromisse erzwingen; Ergebnis offen. |
| CPU / Speicher | Voraussichtlich kleinster Zusatzaufwand; keine Messung. | Zusätzliche Hüllkurven, Peak-Suche, Phasenfortschreibung und Noise-Modulation pro Frame. Fester, vorallokierter Zustand; Laufzeit/Worst Case messen. | Höherer Entwicklungs- und voraussichtlicher Laufzeit-/Speicheraufwand; keine Messung. |
| Echtzeit-Sicherheit | Einfacher zu begrenzen, aber erfüllt die funktionale Muss-Anforderung für Verkürzung nicht. | Echtzeitfähig konstruierbar: feste Bins und Speicher, begrenzte Arbeit pro Frame, parameterweise Glättung. Nachweis durch Review und Instrumentierung nötig; JUCE-Wrapper-Risiko bleibt davon unberührt. | Größere Zustands- und Testfläche; Echtzeitverhalten erst nach Architekturversuch bewertbar. |
| Testbarkeit | Bandkurve messbar, aber kein aussagekräftiger Nachweis für echte Verkürzung oder P1-Artefaktbehebung. | Offline mit Kick, abklingendem Ton und bandbegrenztem Rauschburst prüfbar; Nullpunkt, Abklingkurve, Phasen-/Spektralverhalten, Peak und Laufzeit vergleichbar. | Vergleich erfordert zusätzliche Referenzen und eine größere Testmatrix; als erster Einzelbandversuch schwerer isolierbar. |
| Wechselkosten / Reversibilität | Niedrig; leicht zurückzunehmen, aber das Risiko bleibt ungelöst. | Mittel: neue Tracker, Peak-Locking und Editor im vorhandenen Kern. DSP-Zweig und Parameter können als Prototyp-Slice isoliert und abgeschaltet werden. Keine neue Bibliothek festgelegt. | Hoch: neue Signalzerlegung oder Engine wirkt auf DSP, Latenz, Parameter und Tests. |
| Evidenz / offene Punkte | Vorhandener P1-Code und Auftraggeber-Hörbericht; die geplanten Änderungen sind nicht getestet. | P1-Kern und Hörbericht sind Grundlage. Das Verfahren ist ein Entwurf, kein gemessener Qualitätsnachweis. `bandFreqHz`/`bandQ`-Abbildung, Schwellen und Übertragungsverhalten offen. | Produktvision nennt hybride Spektralverarbeitung; keine Zerlegung oder Multiresolution-Implementierung im vorliegenden Kern nachgewiesen. |

## Vorgeschlagener Ansatz: Option B

### Peak-Band-Gewichtung

Das Band soll durch eine stetige, symmetrische Glocke auf logarithmischer
Frequenzachse gewichtet werden:

`W(k; f0, Q) = exp(-0.5 * (2 Q * log2(fk / f0))^2)`

Dies ist eine illustrative Form, keine freigegebene Q-Kalibrierung.
Eine äquivalente bandbreitennormierte Glocke ist ebenfalls möglich; die
Abbildung von `bandQ` auf Oktavbreite muss festgelegt und hörend geprüft
werden. Sehr kleine Gewichte werden mit glatter Flanke auf 0 begrenzt.

Bei `bandDecay == 0` muss `W_eff` exakt 0 sein und der vorhandene neutrale
WOLA-Pfad unverändert bleiben. Das ist durch direkten Vergleich zum P1-
Neutralpfad zu prüfen; der Anspruch ist noch kein Messergebnis.

### Verkürzung (`bandDecay < 0`)

Vorschlag: gekoppelte pro-Bin-Hüllkurven auf dem Stereo-Link-Betrag führen:
eine schnelle Attack-/Momentanhüllkurve `E_fast(k)` und eine langsamere
Referenz-/Nachklanghüllkurve `E_slow(k)` (alternativ ein Onset-/Sustain-
Tracker). Bei einem Angriff, wenn `E_fast ≈ E_slow`, bleibt der Gain nahe
1, damit Transienten erhalten bleiben. Beim Abklingen senkt ein
zeitabhängiger Gain `g_short(k) ∈ [g_min, 1]` den komplexen Dry-Bin
proportional zu `W(k) * (abs(bandDecay) / 5)` ab. Der Dry-Bin wird nicht
phasenrotiert; es wird kein künstlicher Tail erzeugt.

Die konkrete Tracker-Dynamik, Gain-Kennlinie, `g_min` und deren
Abnahmeschwellen sind offen. Eine reine statische Spektraldämpfung wäre
hingegen ein Lautstärke-EQ: Sie senkt dauerhaft ein Band, statt dessen
Ausklang zeitabhängig zu verkürzen.

### Verlängerung (`bandDecay > 0`)

Die effektive Nachklang-Zeitkonstante `tau(k)` und die Zusatzstärke sollen
stetig mit `W(k) * (bandDecay / 5)` wachsen. Verlängerung bleibt auf das
vorläufige Zwei-Sekunden-Aktivitäts-Timeout und das Sample-Peak-Ceiling
begrenzt. Aktivitätsbezug, Timeout-Messung, Ceiling und Übergänge bleiben
offen; das Ceiling ist kein True-Peak-Nachweis.

- **Tonal (`toneNoise -> 0`):** Lokale Spektralpeaks im aktiven Band suchen
  und benachbarte Bins im Einzugsbereich über Identity/Rigid Phase Locking
  an die Phasenfortschreibung des Peak-Bins binden; ihre relativen
  Analyse-Phasen zum Peak erhalten. Geglätteter Defizit-Einsatz soll
  hopweise Aktivierungssprünge mindern. Dies ist ein zu prüfender
  Verfahrensvorschlag, kein bereits belegter Artefaktnachweis.
- **Rauschhaft (`toneNoise -> 1`):** Frequenzabhängig dämpfen, sodass hohe
  Frequenzen schneller ausklingen als tiefe, und zeitlich/spektral sanft
  korrelierte Zufallsmodulation mit begrenzter Phasenstreuung verwenden,
  statt vollständig unkorrelierter stationärer Phasen. Die gewünschte
  Natürlichkeit ist durch Hör- und Spektraltests zu prüfen.
- **Morph:** `toneNoise` soll Phasenstreuung/Jitter-Tiefe und die
  energieerhaltende Mischung von peak-gelockter tonaler zu gedämpfter
  stochastischer Synthese kontinuierlich steuern. Energieerhalt und
  Monotonie sind zu messen; keine Schwelle dafür ist hier festgelegt.

### GUI und Threading

Vorgesehen ist ein schlanker `juce::AudioProcessorEditor` mit interaktiver
Peak-Band-Kurve: X = logarithmische Frequenz, Y = Decay `[-5,+5]`,
Bandbreite = `Q`. Dazu vier `APVTS-SliderAttachment`s für `bandDecay`,
`toneNoise`, `bandFreqHz` und `bandQ`. Die konkrete Kurven- und
Parameterdarstellung ist Teil des Versuchs.

Kein gemeinsamer Speicherzugriff auf GUI- und DSP-Puffer. Der Audio-Thread
liest nur nichtblockierend ausgetauschte Parameterwerte und glättet sie mit
vorallokiertem Zustand. Die aktuelle atomare Parameterübergabe des
`NeutralStft` ist ein Ausgangspunkt, kein Nachweis für diesen neuen
Parameterumfang oder den gesamten VST3-Callback.

## Vorgeschlagene Entscheidung

**Für Prototyp 2 wird Option B zur experimentellen Umsetzung vorgeschlagen.**
Sie nimmt die drei konkreten Bedienungsbefunde auf, greift die berichteten
P1-Artefaktursachen direkt auf und lässt sich im bestehenden
N=2048/H=512-WOLA-Kern isolieren. Option A ist als Minimaländerung zwar
reversibel, erfüllt aber Verkürzung funktional nicht und untersucht die
gemeldeten Artefakte nicht. Option C ist für einen Einzelband-Prototyp mit
höheren Wechsel-, Latenz- und Validierungskosten unverhältnismäßig.

**Keine Produktentscheidung:** Diese Empfehlung gilt nur für den Versuch.
Produktverantwortung entscheidet nach Windows-/Ableton-Hörtest von
Prototyp 2; DSP und Test beraten anhand der dokumentierten Messungen.
Numerische Abnahmeschwellen bleiben ausdrücklich offen.

## Konsequenzen

- **Positiv:** Bietet einen prüfbaren Weg zu echtem Verkürzen, neutralem
  Nullpunkt, stufenlosem Morph und intuitiver Peak-Band-Bedienung.
- **Negativ / Folgekosten:** Peak-Suche, gekoppelte Tracker und Noise-Morph
  erhöhen DSP-, GUI- und Testumfang. Kein Verfahren garantiert natürliches
  Klingen; weder CPU noch Echtzeitverhalten sind nachgewiesen.
- **Frühe Warnsignale:** Nullpunkt weicht vom neutralen WOLA-Pfad ab;
  Transienten werden trotz negativer Einstellung hörbar abgeschnitten;
  Ton bleibt granular, Noise hallig/stationär, Frequenz oder Stereobild
  instabil; Peak-Überschreitungen, zusätzliche Latenz oder untragbare
  Callback-Zeit.
- **Validierungsschritt:** Kleinster aussagekräftiger Versuch ist ein
  deterministischer Offline-Render im bestehenden DSP-Harness: neutraler
  Vergleich plus ein Kick-Testsignal mit abklingendem Bass, ein
  abklingender Sinus im Peak-Band und ein bandbegrenzter Rauschburst. Für
  mehrere `bandDecay`-Werte beider Vorzeichen sowie `toneNoise` an den
  Endpunkten und mindestens einem Zwischenwert Ausgabe vergleichen;
  Nullpunkt, Zielband-Abklingverlauf, Transienten, Tail-Ende, Peak,
  Sample-Latenz und Zeit pro Callback protokollieren. Klangvergleich
  anschließend im Windows-/Ableton-Prototyp-2-Test; Messwerte ohne
  beschlossene Grenzen nur berichten.
- **Ergänzende Evidenz:** Der oben beschriebene Versuch wurde implementiert
  und unter Linux technisch geprüft. Das ersetzt weder den ausstehenden
  Windows-/Ableton-Hörtest noch die Entscheidung über offene numerische
  Kriterien; siehe [Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md).
- **Folgearbeit:** DSP-Spec, Testsignale und Windows-/Ableton-Testplan
  abstimmen. Neue Abhängigkeiten oder Lizenzänderungen sind nicht Teil
  dieser Empfehlung; falls später vorgeschlagen, separat mit Folgen
  beschreiben und freigeben.

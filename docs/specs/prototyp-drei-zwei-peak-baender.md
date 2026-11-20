# Prototyp 3: Zwei Peak-Bänder mit stärkerer Verkürzung

Status: abgestimmt  
Spec-Stufe: Spec Anchored

## Ziel und Bedarf

Prototyp 3 soll aus dem praktisch interessanten Prototyp 2 ein besser
nutzbares Werkzeug machen. Musikschaffende sollen in einem Signal zwei
Frequenzbereiche unabhängig verkürzen oder verlängern können, während die
Verkürzung deutlich weiter reicht als bisher und den Attack möglichst erhält.

Bezug zur Produktvision: [Produkterlebnis](../product/product-vision.md#produkterlebnis),
[erster Entwicklungs-Build](../product/product-vision.md#erster-entwicklungs-build)
und [Erfolgskriterien](../product/product-vision.md#erfolgskriterien).
Das Ziel bildet einen konkreten Zwischenschritt zum geplanten Mehrpunkt-
Kurveneditor, nicht dessen vollständige Implementierung.

## Belegt aus dem P2-Hörtest

Der Produktverantwortliche testete den neuen P2-Peak-Band-Prototyp in Ableton
Live 12 unter Windows 11. Quellrevision, Samplerate, Puffergröße und weiteres
Testmaterial sind nicht mehr bekannt.

- Snare-Body ließ sich mit kleinem Q bereits sinnvoll verlängern.
- Vocal-Reinigung und Automation waren praktisch interessante Anwendungsfälle.
- Der Attack blieb beim Verkürzen klanglich gut erhalten.
- Am negativen Ende reichte die maximal mögliche Verkürzung häufig nicht aus;
  gewünscht wurde mehr Wirkung als beim bisherigen Wert `-5`.
- Tone war passender als beim vorherigen Stand, aber noch nicht ausreichend
  „klar“ und teilweise leicht granular.
- Noise klang noch zu hallig; gelegentlich entstanden Echo-/Granularartefakte.
  Die genaue Abhängigkeit von Tone/Noise, Q oder Material ist offen.
- Zwei Bänder sollen als nächster Bedienungs- und Nutzenschritt untersucht
  werden. Tone/Noise wird pro Band benötigt.

Diese Aussagen sind qualitative Evidenz aus einem Hörtest, keine formale
Windows-/Ableton-Abnahme und kein quantitativer Qualitätsnachweis.

## Umfang

**Enthalten:**

- Zwei gleichzeitig aktive Peak-Bänder mit je eigener Frequenz, Q/Breite,
  bipolarem Decay und Tone/Noise.
- Beide Bearbeitungsrichtungen gleichzeitig: ein Band kann kürzer und das
  andere länger ausklingen.
- Eine deutlich stärkere negative Wirkung als der bisherige Maximalwert
  `bandDecay = -5`, ohne den Attack unnötig zu beschädigen.
- Klangverbesserung der Verlängerung: klarerer Tone-Anteil und weniger
  halliger Noise-Anteil.
- Rückfall auf neutral, wenn beide Decay-Werte 0 sind.
- Editor, Parameterzustand, Offline-/Processor-/VST3-Tests und Ableton-
  Hörvergleich für zwei Bänder.
- Vor Beginn tiefgreifender Änderungen einen reproduzierbaren Archivstand von
  Prototyp 2 bewahren; der P1-Archivpfad bleibt unverändert.

**Nicht enthalten:**

- Klassische Low-/High-Pass-Filter, Shelves oder zusätzliche Randbänder.
- Freie Anzahl von Punkten, Punkte hinzufügen/löschen oder ein beliebig
  interpolierter Mehrpunkt-Kurveneditor.
- Spektralanzeige, Presets, Dry/Wet, eigener Bypass oder vollständige
  Produktreife.
- Neue Drittanbieter-Abhängigkeiten oder eine neue Engine-Architektur.
- Verpflichtende Zusage einer bestimmten Millisekunden-Abklingzeit.

**Betroffene Komponenten:**

DSP-Kern, Bandgewichtung und -kombination, Parameter/APVTS, Plugin-Zustand,
Peak-Band-Editor, Tests und Windows-/Ableton-Prüfprotokoll.

## Prioritäten

Bei Zielkonflikten gilt für Prototyp 3:

1. den Attack eines transienten Signals beim Verkürzen erhalten;
2. die erreichbare Verkürzung deutlich über P2 hinaus erweitern;
3. granulare, hallige und echoartige Artefakte reduzieren.

Die Priorität bedeutet nicht, dass eine schwache Verkürzung akzeptabel ist.
Sie verlangt zunächst, zusätzliche Wirkung durch zeitabhängige Tail-Steuerung
statt durch statische Pegelabsenkung oder Attack-Beschneidung zu erreichen.

## Akzeptanzkriterien

### AK1 – Stärkere, attackschonende Verkürzung

Gegeben ein reproduzierbares Kick-/Snare-ähnliches Testsignal im gewählten
Frequenzband bei 44,1 und 48 kHz: Wenn ein Band über den bisherigen P2-Wert
`-5` hinaus in den neuen Bereich bewegt wird, dann sinkt der gemessene
Ausklangsanteil stärker als bei P2 `-5`. Der initiale Attack darf nicht stärker
als in einer vorab festgelegten Referenz beeinträchtigt werden.

Messfenster, Ausklangsmetrik, neuer Minimalwert und Attack-Toleranz sind offen.
Ein rein statischer Pegelabfall des ganzen Bands erfüllt das Kriterium nicht.

### AK2 – Zwei unabhängige Bearbeitungsrichtungen

Gegeben ein Signal mit einem tiefen, abklingenden Anteil und einem getrennten,
höheren abklingenden Anteil: Wenn Band 1 negativ und Band 2 positiv
konfiguriert ist, dann wird der tiefe Ausklang verkürzt und der hohe Ausklang
verlängert. Frequenzen außerhalb der wirkenden Bänder dürfen nicht unerwartet
abgeschnitten werden.

Die genaue Überlagerungsregel bei sich überschneidenden Glocken, Mindestwirkung
und numerische Selektivitätstoleranzen sind offen.

### AK3 – Tone/Noise pro Band

Gegeben zwei verlängerte Bänder: Wenn der Tone/Noise-Wert nur in einem Band
verändert wird, dann ändert sich die Fortführung nur dieses Bands messbar und
hörbar. Das zweite Band bleibt bei unveränderten Parametern unverändert.

Der stufenlose Übergang muss weiterhin endlich und während Automationsänderung
frei von unvertretbaren binären Sprüngen sein. Klick-/Stetigkeitstoleranzen
sind offen.

### AK4 – Klangverbesserung gegenüber P2

Gegeben die in P2 verfügbaren Ton-, Noise- und Mischtestsignale: Dann muss P3
in einem dokumentierten A/B-Hörtest gegen P2 mit denselben Grundeinstellungen
beurteilt werden. Positiv ist P3, wenn Tone klarer und Noise weniger hallig
wirkt, ohne die bereits praktisch nutzbare Snare-Verlängerung zu verlieren.

Ein Material-/Einstellungsprotokoll und die Entscheidung, welche Signale die
Referenz bilden, sind offen. Blindvergleich wird empfohlen, ist aber noch kein
verbindlicher Grenzwert.

### AK5 – Neutralität, Latenz und Zustand

Wenn beide Decay-Werte neutral sind, dann bleibt der bestehende
latenzkompensierte P2-Referenzpfad ohne zusätzliche Bearbeitung bestehen. Die
gemeldete Latenz muss weiterhin der gemessenen Latenz entsprechen. Beide
Bänder mit sämtlichen vier Parametern werden gespeichert und wiederhergestellt;
leere, korrupten und ältere Zustände fallen sicher auf gültige Defaults zurück.

Die bestehende Nullrestgrenze bleibt offen. Die konkrete Versionsmigration von
P2-Zustand zu P3 muss in den Tests sichtbar geprüft werden.

### AK6 – Echtzeit und Robustheit

Bei Mono/Stereo, 44,1/48 kHz, Blockgrößen 1, 64, 257 und 512 sowie
automatisierten Parametern bleibt die Ausgabe endlich und blockgrößeninvariant.
Der eigene DSP-Callback darf weder allokieren noch sperren, I/O ausführen oder
loggieren. Die Arbeit je Frame bleibt begrenzt.

CPU-/Worst-Case-Grenzen und numerische Blockinvarianz-Toleranzen sind offen.
Das bekannte JUCE-Wrapper-Lock bleibt als separates Gesamtsystemrisiko
dokumentiert.

### AK7 – Bedienung zweier Bänder

Beide Bandpunkte sind während der Wiedergabe direkt editierbar. Die jeweils
zugehörigen Frequency-, Width/Q-, Decay- und Tone/Noise-Steuerungen müssen
räumlich und visuell eindeutig am aktiven Band erkennbar sein, damit bei mehr
Parametern nicht unnötig zwischen räumlich entfernten Reglergruppen gewechselt
werden muss.

Die konkrete Darstellung – zwei Kontextreglersätze, ein gemeinsam genutztes
Panel oder eingebettete Popup-Steuerungen – ist eine offene GUI-Entscheidung.
Sie muss an einem Ableton-Bedienversuch mit mehreren Parametern bewertet
werden.

### Grenz- und Fehlerfälle

- Überlappende Bänder und gleiche Mittenfrequenz
- ein Band neutral, das andere aktiv
- stärkste Verkürzung und stärkste Verlängerung
- Automationssprünge, Übergänge und Stille
- clipping-nahe Eingangssignale
- Mono-, Stereo- und gegenphasiges Material
- Sampleraten-/Blockgrößenwechsel
- P2-, leerer und beschädigter Plugin-Zustand

## Geplanter Umsetzungsweg

1. **P2-Basis sichern:** aktuellen getesteten Quellstand und seine Messbasis
   festhalten; erst danach den Hauptbaum für P3 ändern.
2. **Verkürzung isoliert untersuchen:** stärkeren negativen Bereich zuerst mit
   dem vorhandenen einzelnen Band erproben; Attack- und Ausklangmessung gegen
   P2 `-5` vergleichen.
3. **DSP auf zwei Bänder erweitern:** unabhängige Zustände, Bandgewichtung,
   Kombination und je Band eigener Tone/Noise-Zweig.
4. **Parameter und Zustand erweitern:** eindeutige IDs pro Band und sichere
   Migration vom P2-Einbandzustand.
5. **Editor erweitern:** Auswahl beider Bänder und kontextnahe Parameter;
   Layout zunächst als reversibler Versuch.
6. **Tests erweitern:** bestehende Null-, Richtungs-, Stereo-, Tail-, State-
   und Hosttests auf zwei Bänder und P2-Migration erweitern.
7. **P2/P3-Hörvergleich:** Snare, Kick, Vocals, abklingender Ton,
   bandbegrenztes Rauschen und ein komplexes Beispiel unter Windows/Ableton
   dokumentieren.

Bei der Fortsetzung wird mit diesem Arbeitsauftrag begonnen; es ist nicht
nötig, die Interviewentscheidungen erneut zu sammeln.

## Qualität

Relevante Szenarien:

- [QS-01/QS-02](../quality/szenarien.md#qs-01--neutraler-durchlauf)
- [QS-04](../quality/szenarien.md#qs-04--begrenzter-nachklang)
- [QS-05](../quality/szenarien.md#qs-05--echtzeitverhalten-der-stft-fft)
- [QS-07](../quality/szenarien.md#qs-07--bipolarer-decay-regler)
- [QS-08](../quality/szenarien.md#qs-08--stufenloser-tonenoise-übergang)
- [QS-09](../quality/szenarien.md#qs-09--eq-peak-band-steuerung)

Für P3 kommen insbesondere Bandüberlagerung, unabhängiger Tone/Noise-Zweig und
angriffsbezogene Verkürzungsstärke hinzu. Verbindliche numerische
Qualitätsschwellen bleiben offen.

## Kontext und Evidenz

**Maßgebliche Dateien:**

- [Produktvision](../product/product-vision.md)
- [Produkt-Backlog](../product/product-backlog.md)
- [Spec Prototyp 2](prototyp-zwei-peak-band.md)
- [Prüfbericht Prototyp 2](../verification/peak-band-prototype-2.md)
- [ADR 0005](../adr/0005-zweiter-prototyp-bipolares-peak-band.md)

**Annahmen:**

- Zwei Bänder bieten genügend zusätzlichen Nutzen, ohne schon den kompletten
  Mehrpunkt-Editor zu erzwingen.
- Eine breitere negative Skala kann über die zeitabhängige Tail-Steuerung
  erweitert werden, ohne den neutralen Pfad oder den Attack aufzugeben.
- Per-Band-Tone/Noise ist musikalisch nützlich; dies muss im Hörvergleich
  bestätigt werden.

**Offene Fragen mit Produktverantwortung:**

- Wie weit soll die negative Skala reichen, und bleibt `0` bis `+5` unverändert?
- Wie wird „Attack erhalten“ gemessen und welche Verschlechterung ist noch
  zulässig?
- Wann ist die zusätzliche Verkürzung hörbar ausreichend?
- Wie werden überlappende Bänder kombiniert?
- Welche Testsignale und A/B-Einstellungen bilden den verbindlichen
  Hörvergleich?

## Verifikation

**Automatisiert:**

- P2-Referenz und P3-Neutralpfad latenzkompensiert vergleichen.
- Attack- und Tail-Energie getrennt für P2 `-5` und mehrere neue negative
  Versuchswerte messen.
- Mehrband-Signal mit gegensätzlicher Richtung auf Bandselektivität prüfen.
- Per-Band-Tone/Noise-Unabhängigkeit und Automation auf Stetigkeit prüfen.
- Überlappende Bänder, Stille, Peaks, Stereo, alle vorgesehenen Blockgrößen
  und Zustandsmigration prüfen.
- Relevante CTest-Programme und, falls verfügbar, Plugin-Validierung ausführen.

**Manuell / Windows-Ableton:**

- P2 und P3 mit denselben Snare-, Kick-, Vocal-, Ton-, Rausch- und
  Mix-Ausschnitten vergleichen.
- Primär prüfen: Kann der Ausklang stärker verkürzt werden, ohne den Attack
  zu zerstören?
- Sekundär prüfen: Ist Tone klarer, Noise weniger hallig und sind Echo-
  Artefakte seltener?
- Das Arbeiten mit zwei Bändern und deren kontextnahen Parametern beurteilen.
- Windows-, Ableton-, Build-, Samplerate-, Puffer-, Bus- und Materialdaten
  protokollieren.

## Fortsetzen dieser Arbeit

Auftrag bei der nächsten Aufforderung „weiter machen“, „nächsten Schritt
umsetzen“ oder „Stand anzeigen“:

1. Diese Spec und den unten verlinkten Stand im Backlog lesen.
2. Das erste noch nicht erledigte Paket aus „Geplanter Umsetzungsweg“
   identifizieren.
3. Vor Produkt-/GUI-/DSP-Entscheidungen die offenen Fragen auf eine konkrete
   nächstgelegene Entscheidung prüfen.
4. Nur das kleinste reversible Paket umsetzen und mit den dafür passenden Tests
   beenden.
5. Danach Spec, Backlog und Prüfbericht aktualisieren; noch offene oder nicht
   ausgeführte Checks ausdrücklich nennen.

## Freigaben

- Agent darf: Repository lesen, Spec und zugehörige Dokumentation ändern,
  lokale Builds/Tests ausführen und kleine reversible P3-Schritte im Rahmen
  dieser Spec umsetzen.
- Braucht Zustimmung: neue Abhängigkeiten, schwer umkehrbare Engine- oder
  Pluginformatentscheidung, Veröffentlichung, Push, Release, externe Dienste
  oder Abweichung von den vereinbarten zwei Bändern mit pro-Band-Tone/Noise.

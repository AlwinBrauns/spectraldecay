# 0001 – Architektur für den neutralen Prototyp

- **Status:** akzeptiert
- **Datum:** 2026-11-15
- **Entscheider:** Produktverantwortung, Freigabe am 2026-11-15
- **Bezug:** [PB-01/PB-02](../product/product-backlog.md#vorgeschlagene-reihenfolge), [Spec neutraler Durchlauf](../specs/neutraler-durchlauf.md), [QS-01/QS-02](../quality/szenarien.md), [Lizenzoptionen](0002-lizenzmodell.md)

**Implementierungsstand:** Der neutrale STFT-Untersuchungspfad ist inzwischen
mit FFT 2048/Hop 512 sowie gepinnten JUCE-/PFFFT-Quellen umgesetzt; Linux-
Build und Offline-/Processor-/VST3-Tests sind erfolgreich. Das belegt den
Prototyp, nicht die langfristige Produktarchitektur oder offene
Neutralitätsgrenzen. Der aktuelle Verlängerungsversuch ist im
[Prüfbericht](../verification/tail-prototype.md) beschrieben.

## Kontext

**Belegt zum Entscheidungszeitpunkt:** Der erste Entwicklungs-Build soll als
JUCE/C++-VST3 in Ableton Live 12 unter Windows laufen. Ein neutraler
spektraler Durchlauf ist ein Zwischenschritt; Verkürzung und Verlängerung
sind für den vollständigen Build notwendig. Die Produktverantwortung hat
STFT als Untersuchungspfad und Windows x86-64 als Testplattform freigegeben.
Konkrete Abhängigkeiten und Messgrenzen waren damals offen.

**Ziel dieses ADR:** Eine Grundlage für den separat zu beauftragenden
neutralen Prototyp auswählen, ohne das Verlängerungsverfahren oder das
fertige Produkt vorwegzunehmen. Beide Optionen sind zunächst Hypothesen; es
gibt weder Code noch Messungen. Ableton unterstützt unter Windows VST3 und
seit Live 10 nur 64-Bit-Plugins
([Ableton](https://help.ableton.com/hc/en-us/articles/209071729-Using-VST-plug-ins-on-Windows)).

## Optionen

| Kriterium | A: STFT mit gewichteter Overlap-Add-Synthese | B: rekonstruktive Mehrband-Filterbank | Evidenz / offene Frage |
| --- | --- | --- | --- |
| Muss-Anforderungen | Spektraler neutraler Pfad grundsätzlich möglich; beide Decay-Richtungen noch zu entwickeln. | Frequenzbänder und Neutralpfad grundsätzlich möglich; Eignung als geforderter spektraler Pfad und für beide Richtungen gesondert klären. | Beides nicht implementiert oder nachgewiesen. |
| Audioqualität / Artefakte | Bei korrekter Fensterung und Normierung ist neutrale Rekonstruktion prinzipiell möglich. Bearbeitung kann Phasing, metallische Töne, Pre-Echo und Transientenverschmierung erzeugen. | Rekonstruktion und Phasenverhalten hängen von Filtern und Bandübergängen ab; dynamische Bearbeitung kann Phasen- und Übergangsartefakte erzeugen. | **Annahmen:** konkrete Fehler und Höreindruck erst messen. |
| Latenz / CPU / Speicher / Echtzeit | Fensterlänge und Hop beeinflussen Auflösung, Reaktionszeit, Latenz und Rechenlast. Feste, vorallokierte Puffer und begrenzte Arbeit pro Block vorsehen. | Filterordnung und Bandzahl beeinflussen Auflösung, Latenz, Arbeit und Speicher; ebenfalls begrenzte Verarbeitung und vorallokierte Puffer nötig. | Keine Werte oder Echtzeittauglichkeit nachgewiesen; Latenz nicht pauschal aus der Fensterlänge ableiten. |
| Testbarkeit | Rekonstruktion durch Offline-Nulltest und gemeldete versus gemessene Impulslatenz prüfbar; Blockgrenzen und Ein-/Ausschwingen erfassen. | Dieselben Tests sind möglich; zusätzlich Bandübergänge und Phasenbeziehungen untersuchen. | [Spec AK1–AK5](../specs/neutraler-durchlauf.md#akzeptanzkriterien); Schwellen und Auswertung offen. |
| Abhängigkeiten / Wechselkosten | JUCE und FFT-Implementierung benötigen Lizenzprüfung; STFT ist eine Grundlage, aber keine fertige Verlängerung. | Filterbank-Implementierung und ihre Lizenz sind zu prüfen; spätere Änderung von Bandaufteilung oder Engine beeinflusst Tests und Parameterabbildung. | Konkrete Versionen, FFT und Filterdesign offen; keine zusätzlichen Bibliotheken freigegeben. |
| Warnsignal / Umkehrbarkeit | Hohe gemessene Latenz, Rekonstruktionsfehler oder hörbare Artefakte; Wechsel der Engine ist möglich, aber teuer. | Keine ausreichend transparente Rekonstruktion oder ungeeignete Bandauflösung; Wechsel ist möglich, aber teuer. | Mit neutralem Prototyp allein ist keine Bewertung der Verlängerungsqualität möglich. |

## Entscheidung

**Freigegeben:** Option A nur als neutralen Untersuchungspfad verfolgen. Sie
passt zur bereits beschriebenen hybriden
STFT-Richtung der Vision und ermöglicht einen frühen Rekonstruktions- und
Latenznachweis. Das ist keine Aussage, dass sie klanglich überlegen ist.
Fenster, Hop, FFT-Größe und ihre Normalisierung sind vor der Implementierung
für den Versuch zu begründen; sie sind hier nicht festgelegt. Eine
Hüllkurven-, Transienten- oder Verlängerungsstrategie wird nicht entschieden.

**Zuschnitt des Untersuchungspfads:**

- Plattformunabhängiger C++-DSP-Kern, von JUCE-Plugin-/Hostanbindung und
  Bedienoberfläche getrennt; derselbe Kern dient Offline- und Host-Tests.
  VST3 und CMake mit JUCE sind Kandidaten, keine bereits bezogenen oder
  freigegebenen Abhängigkeiten. Konkrete Versionen und FFT-Quelle erst nach
  Lizenzprüfung auswählen.
- Audio-Thread: vorallokierte Puffer, keine Locks, Allokation oder I/O;
  beliebige Host-Blockgrenzen und 0/1 Sample berücksichtigen. GUI- und
  Audio-Thread nur über geeigneten nicht blockierenden Austausch verbinden.
  Die tatsächliche Latenz messen und an den Host melden.
- Später für Dry/Wet den Dry-Pfad passend verzögern. Der Ausgangsschutz
  darf bei neutraler Einstellung eingreifen, ist jedoch bei Bypass und
  100 % Dry nicht aktiv. Bypass-Latenz und Umschaltverhalten bleiben offen.
- Zielplattform ist **Windows x86-64**; keine Zusage für Windows ARM oder
  andere Plattformen. Die Produktverantwortung bestätigt, dass eine
  Windows-x86-64-Umgebung mit Ableton Live 12 verfügbar ist. Dies ist eine
  Auswahl für die erste Testmaschine, nicht eine Aussage, dass Windows
  ausschließlich x86-64 unterstützt.

## Konsequenzen

- **Positiv:** Der neutrale Signalweg ist unabhängig von der späteren
  Bearbeitung messbar; die Plugin-Hülle kann auf anderen Plattformen
  grundsätzlich wiederverwendet werden.
- **Negativ / Folgekosten:** Fenster-/Latenzkonflikt bleibt; ein erfolgreicher
  Nulltest beweist weder Decay-Wirkung noch Stereoqualität unter Bearbeitung.
  Ein möglicher Engine-Wechsel betrifft DSP, Tests und spätere
  Parameterabbildung.
- **Frühe Warnsignale:** instabile oder nicht rekonstruktive Ausgabe,
  Abweichung der gemeldeten von der gemessenen Latenz, Blockgrößenabhängigkeit
  oder untragbare Rechenlast. Grenzwerte für Neutralität und CPU sind offen;
  Messwerte bis zur Festlegung nur berichten.
- **Folgearbeiten:** Lizenz und Versionen freigeben, danach gesonderten
  Prototypauftrag erstellen; Offline-Messungen gemäß Spec und
  Echtzeitprüfung durchführen. Die Zwei-Sekunden-Tailbegrenzung,
  Stereo-Kopplung, True-Peak-Verfahren und Übergänge brauchen eigene
  Entscheidungen und Tests.

### Vorgeschlagener Windows-/Ableton-Testweg

Nach Implementierungsfreigabe einen x86-64-VST3-Build auf einer bestätigten
Windows-Testmaschine erzeugen und installieren; Windows-Version,
Prozessorarchitektur, Build/Version, Ableton-Version, Samplerate, Host-
Blockgröße, Kanallayout und Installationspfad protokollieren. In Live den
VST3-Scan aktivieren, das Plugin laden und mit Mono- und Stereo-Quellmaterial
prüfen. Welche Kanallayouts Live tatsächlich an das Plugin übergibt, ist
gesondert festzuhalten; Mono-Material allein beweist kein Mono-Plugin-Layout.
Ein bekanntes Signal bei neutralem spektralem Pfad durchleiten und Audio sowie
gemeldete Latenz mit einem geeigneten Host-Test bzw. einer dokumentierten
Messung prüfen. Zusätzlich beim später vollständigen Build Hörtests und
Schutz-Ausnahmen in Live prüfen. Testsignal, Ergebnis und etwaige Fehler
festhalten.

Offline-Nulltest, Latenz- und Blockgrößenmessung gemäß Spec ergänzen den
Hosttest; ein Linux-Build ersetzt den Windows-/Ableton-Nachweis nicht.
Test-Host und exakte Prüfmethode sind noch auszuwählen. Dieser Ablauf ist ein
Plan, kein ausgeführter Test.

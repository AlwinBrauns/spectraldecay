# 0004 – Kleinster reversibler Verlängerungsprototyp

- **Status:** vorgeschlagen
- **Datum:** 2026-11-15
- **Entscheider:** Produktverantwortung; DSP-Entwicklung und Test beraten
- **Bezug:** [Produktvision](../product/product-vision.md#technische-richtung-und-risiken), [Backlog PB-03/PB-04](../product/product-backlog.md#vorgeschlagene-reihenfolge), [ADR 0001](0001-prototyp-architektur.md), [neutraler STFT-Kern](../../src/dsp/NeutralStft.cpp)

## Entscheidungsfrage und Muss-Anforderungen

Welcher kleinste, hörbare und leicht rücknehmbare DSP-Versuch soll auf dem
vorhandenen neutralen STFT-Kern als Nächstes untersucht werden, um das in der
Vision benannte Verlängerungsrisiko zu klären, ohne Engine-Architektur oder
Abhängigkeiten grundsätzlich zu ändern?

**Belegt:** `NeutralStft` verwendet N=2048, Hop=512, sqrt(periodische Hann),
PFFFT und WOLA. Die neutrale Verarbeitung meldet 2048 Samples Latenz; das
Prüfprotokoll berichtet dieselbe Impulslatenz in den ausgeführten
Offline-/Linux-Prüfungen. Der Kern hat noch keine Decay-Verarbeitung. Die
Vision sieht STFT/Hüllkurvenverfolgung fürs Verkürzen und Fortführung
tonaler/rauschhafter Anteile fürs Verlängern vor. PB-03 und PB-04 sind offen;
der vollständige erste Build soll beide Richtungen in verschiedenen
Frequenzbereichen zugleich hörbar machen.

Für **diesen Versuch**, nicht als Abnahme des vollständigen Builds, gelten:

- Die bestehende FFT-, WOLA- und Pufferarchitektur sowie die vorhandene
  Abhängigkeitsmenge bleiben erhalten.
- Es gibt einen festen, einzelnen Testfrequenzbereich; keine UI-Kurve oder
  neue Plugin-Bedienung.
- Der neutrale Zustand bleibt unverändert. Der Versuch muss sich mit einem
  abgeschalteten optionalen Bearbeitungsschritt zurücknehmen lassen.
- Verarbeitung im DSP-Callback bleibt begrenzt; dort keine eigene
  Allokation, Sperre, I/O, Protokollierung oder Ausnahme.
- Vorhandene STFT-Latenz soll nicht durch zusätzliche Look-ahead-Frames
  steigen. Latenz und Laufzeit sind dennoch erneut zu messen.
- Ein hörbarer frequenzabhängiger Effekt ist das Versuchsziel. Verbindliche
  Artefakt-, CPU- und Hörschwellen sind offen und werden nicht vorweggenommen.

Das ist kein Auftrag, PB-03/PB-04 gemeinsam fertigzustellen. Insbesondere
werden Kurveneditor, automatische Tonal-/Rauschtrennung, Stereo-Kopplung,
Ausgangsschutz und Produkt-Stilleerkennung nicht entschieden.

## Optionen

Die Muss-Anforderungen sind Ausschlusskriterien. Unter den prototypischen
Optionen wird vorrangig danach abgewogen, ob sie das größte bekannte Risiko
der Verlängerung mit möglichst wenig Eingriff und gut isolierbaren
Hörversuchen untersucht. Es werden keine ungewichteten Punktzahlen vergeben.

| Kriterium | A: Frequenzselektives Verkürzen zuerst | B: Begrenzte Ein-Band-Verlängerungsprobe |
| --- | --- | --- |
| Muss-Anforderungen | Passt in den bestehenden STFT-Pfad; ein neutraler Wert kann den Zusatzschritt auslassen. Erfüllt jedoch nicht das Versuchsziel, die Verlängerungsrisiken zu untersuchen. | Passt als optionaler Schritt zwischen Vorwärts-FFT und inverser FFT in denselben STFT-Pfad. Erfüllt das Versuchsziel, sofern der Neutralpfad unverändert bleibt und keine neue Abhängigkeit hinzukommt. |
| Konkretes Verfahren | Pro Frequenzbin eine geglättete Magnitudenreferenz verfolgen und als spektralen Abwärts-Expander verwenden: Gain höchstens 1, Stärke bei neutralem Wert 0. Das verkürzt abfallende Anteile in einer festgelegten Bandmaske; die Phase bleibt erhalten. | Eine feste Bandmaske und zwei getrennte, manuell ausgewählte Testmodi: **tonal** – aus den letzten Spektralframes die Bin-Phasenfortschreitung schätzen und sie nach dem Tail-Start fortsetzen; **rauschhaft** – die letzte Band-Magnitudenhüllkurve mit reproduzierbar pseudozufälligen Phasen fortführen. Beide Magnitudenverläufe fallen exponentiell ab und enden spätestens an der vorläufigen Zwei-Sekunden-Grenze. |
| Audioqualität / Artefaktrisiko | Geringeres Eingriffsrisiko als bei Synthese, aber spektrale Pump-/Gate-Effekte und Transientenveränderung möglich. Sagt nichts über metallische oder stationäre Verlängerungsartefakte aus. | Untersucht direkt Phaseninstabilität und stationären Rauschcharakter. Möglich sind metallische Töne, Phasenfehler, künstlich gleichförmige Tails, Bandgrenzen und Überhöhungen. Die getrennten Modi testen keine automatische Zerlegung gemischter Signale. |
| Latenz | Bei frameweiser Zustandsführung keine zusätzliche Look-ahead-Latenz zu erwarten; die Änderung muss am Impuls geprüft werden. | Bei Fortführung nur aus vergangenen Frames ist keine zusätzliche algorithmische Look-ahead-Latenz beabsichtigt. Die bestehende 2048-Sample-STFT-Latenz bleibt die Ausgangshypothese. Eine Silenzeschwelle bzw. Tail-Auslösung kann den hörbaren Beginn verzögern, auch ohne zusätzliche Host-Latenz. |
| CPU / Speicher / Echtzeit | Wenig Zusatzarbeit pro Bin; Hüllkurvenspeicher vorab anlegen. Laufzeit und Echtzeitverhalten sind ungemessen. | Zusätzlicher Phasenzustand bzw. PRNG-Zustand pro aktivem Bin und laufende Magnituden-/Phasenarbeit. Nur aktive Bins bearbeiten und alle Zustände in `prepare` anlegen. Phasenberechnung kann CPU kosten; keine Messung liegt vor. |
| Testbarkeit | Gut mit Mehrton-/Rauschsignalen und Vergleich der Abklinghülle; klärt PB-03. | Gut isolierbar mit einem abklingenden Sinus und einem separaten bandbegrenzten Rauschburst; Tail-Energie, Frequenzverlauf, Ende, Peak und Hörartefakte erfassen. Das ist ein Komponentenversuch, kein Nachweis für komplexe Gemische. |
| Wechselkosten / Reversibilität | Niedrig; der zusätzliche Abwärts-Expander lässt sich lokal abschalten. | Niedrig bis mittel; optionaler Synthesezweig und begrenzter Zustand im vorhandenen Kern. Abschalten des Zweigs erhält den neutralen Pfad. Keine FFT- oder Plugin-Architekturänderung; dennoch erhöht jeder dauerhafte Zustand Review- und Testaufwand. |
| Evidenz / offene Punkte | Die Vision nennt Hüllkurvenverfolgung als Richtung fürs Verkürzen; ein entsprechender Algorithmus ist nicht im Kern implementiert oder gemessen. | Die Vision benennt Fortführung tonaler/rauschhafter Anteile als Richtung, nicht als validiertes Verfahren. Der bestehende Kern kann Frames nur bei fortlaufenden Callback-Aufrufen verarbeiten; Verhalten bei Host-Suspendierung/Tail-Meldung und die Stilleerkennung sind nicht belegt. |

## Vorgeschlagener Versuch

**Option B wird als kleinster nächster Risikoversuch vorgeschlagen.** Sie
konzentriert sich auf eine einzelne, im Test festgelegte Bandmaske und
untersucht die riskantere Verlängerung. Die tonal- und rauschhafte Variante
wird jeweils mit einem passenden Einzelsignal geprüft, nicht automatisch
klassifiziert oder in einem Mischsignal getrennt.

Der DSP-Schritt arbeitet mit den bereits geordneten STFT-Spektren. Als
**illustrative Versuchsparameter**, nicht als Produktvorgabe, werden eine
glatte feste Bandmaske von 500–1500 Hz und `tau = 250 ms` vorgeschlagen.
Die Maske ist breit genug, um im vorhandenen 2048-Punkte-FFT-Pfad mehrere
Bins zu umfassen. Beim Versuchs-Tail-Start werden Betrag und Phase des
Zielbereichs festgehalten.

Für den **tonalen** Modus wird im Zielband der stärkste Peak `p` bestimmt.
Aus den letzten Frames wird seine Frequenz mit
`delta = wrap(phi[p,m] - phi[p,m-1] - 2*pi*p*H/N)` und
`omega = 2*pi*p/N + delta/H` geschätzt. Die Synthesephase läuft dann mit
`phiSynth[p,m+1] = phiSynth[p,m] + H*omega` weiter; die relativen Phasen
der benachbarten Peak-Bins zum Peak werden beim Tail-Start fixiert
(einfaches Phase-Locking). Für den **rauschhaften** Modus bleiben die
gehaltenen Bin-Beträge je Frame in ihrer spektralen Form erhalten, erhalten
aber unabhängig fortgeschrittene, deterministisch aus einem vorab
initialisierten PRNG erzeugte Phasen. In beiden Modi fällt jeder
Zielbandbetrag nach `A[k,m] = A0[k] * exp(-t[m] / tau)` ab. Die Synthese
bleibt auf die glatte Bandmaske begrenzt und endet spätestens an der
vorläufigen Zwei-Sekunden-Grenze. Ein ein-Hop-Cosinusübergang am
Tail-Start ist ein weiterer Hörversuchsparameter, keine Produktvorgabe.
Die Tail-Energie wird nicht angehoben;
eine geeignete Summierung mit gleichzeitig anliegendem Eingang und deren
Pegelbegrenzung sind nicht Teil dieses ersten isolierten Versuchs.

Für reproduzierbare Probe-Signale kann der Tail-Start zunächst im
Offline-Versuchsaufbau bekannt sein. Eine automatische Erkennung von Stille
ist ausdrücklich nicht Teil dieser Entscheidung: die Produktvision und das
Backlog markieren sie als offen. Vor einer Plugin-Integration muss ein
Mensch festlegen, wie die vorläufige Zwei-Sekunden-Grenze ausgelöst und
gemessen wird. Das Tail-Ende soll im Versuch spätestens bei dieser Grenze
auf null ausgeblendet sein; Verfahren, Toleranz und Messfenster bleiben
offen.

**Echtzeit- und Latenzrisiken:** Alle Speicher-/Phasen-/Zufallszustände
müssen vorab angelegt sein; keine dynamischen FFT-Größen oder
blockierenden Übergaben. Die zusätzliche Arbeit fällt an Framegrenzen an
und muss gegen die jeweilige Host-Blockdauer gemessen werden. Der Kern hat
derzeit 2048 Samples gemeldete und gemessene Latenz (ca. 42,67 ms bei 48 kHz,
46,44 ms bei 44,1 kHz). Ein Vergangenheitszustand allein sollte diese nicht
erhöhen, doch die nichtneutrale Ausgabe und Tail-Auslösung sind noch nicht
gemessen. Die vorhandene Dokumentation weist außerdem auf einen Lock im
JUCE-VST3-Wrapper vor dem eigenen Callback hin; dieser DSP-Versuch löst
dieses frameworkseitige Echtzeitrisiko nicht. CPU-Worst-Case,
Callback-Allokationen und Lock-Verhalten sind im vorhandenen Nachweis nicht
instrumentiert.

## Qualitätsszenario (Entwurf nach Skill `qualitaetsszenario`)

| Feld | Inhalt |
| --- | --- |
| ID / Qualitätsmerkmal | QS-06 / Audioqualität und Leistungseffizienz – Verlängerungsprobe |
| Auslöser | Tail-Start nach einem bandbegrenzten Sinus bzw. bandbegrenzten Rauschburst |
| Umgebung | Offline-DSP-Probe; bestehende 44,1/48-kHz-STFT-Konfiguration, Mono; Host-Blockgrößen wie im bestehenden Versuch, zusätzlich Echtzeitmessung erst nach gesondertem Auftrag |
| Artefakt | Optionaler Verlängerungszweig im neutralen STFT-Kern |
| Erwartete Reaktion | Im gewählten Band ist ein zusätzlicher, abklingender Tail hörbar; tonal bleibt die geschätzte Frequenzentwicklung nachvollziehbar, rauschhaft bleibt der Tail rauschähnlich. Der Tail wird gemäß der vorläufigen Zwei-Sekunden-Regel begrenzt. |
| Messgröße | Zielband-RMS über der Zeit, Tail-Dauer, Frequenz-/Phasenverlauf des Tons, Spektralverteilung des Rauschens, Peak/True-Peak und Zeit pro Callback-Block |
| Zielwert | Vorläufig höchstens zwei Sekunden ab vereinbartem Stillebezug; alle anderen Schwellen offen – nach dem Prototyp festzulegen |
| Priorität / Risiko | Hoch: Verlängerung ist laut Vision das größte Entwicklungsrisiko; Artefakt-, CPU- und Pegelgrenzen sind offen |
| Prüfung | Getrennte deterministische Sinus- und Rauschproben plus dokumentierter Hörvergleich; Grenzwerte vor einer Abnahme durch Produktverantwortung/Test festlegen |

## Konsequenzen und offene Punkte

- **Positiv:** Der Versuch untersucht das größte ausdrücklich benannte Risiko
  in einem engen, rücknehmbaren Slice und nutzt FFT, PFFFT, WOLA und
  Abhängigkeiten des bestehenden Kerns weiter.
- **Negativ / Folgekosten:** Er liefert weder gleichzeitiges Verkürzen und
  Verlängern noch die automatische Fortführung gemischter tonaler und
  rauschhafter Anteile. PB-03 und vollständiges PB-04 bleiben offen.
- **Warnsignale:** instabile Tonhöhe/Phase, hörbar stationäres oder
  metallisches Rauschen, Tail-Peaks über dem Bezugssignal, Abhängigkeit vom
  Blockraster, zusätzliche Host-Latenz oder überlange Callback-Zeit.
- **Offen:** Bandgrenzen, Tail-Zeitkonstante, Phasen- und Noise-Verfahren im
  Detail, Auslöse-/Stilledefinition, Pegelbegrenzung, Messfenster und
  Akzeptanzschwellen. Die genannten zwei Sekunden sind ein vorläufiges
  Produktziel, kein nachgewiesener Grenzwert.
- **Nächster Validierungsschritt:** Der kleinste aussagekräftige Schritt ist
  ein einzelner Offline-Render durch den bestehenden DSP-Kern mit einem
  abklingenden Sinus in der festen Zielbandmaske, anschließend ein
  bandbegrenzter Rauschburst. Neutralzustand und Tail-Zweig direkt
  vergleichen; Tail-RMS, Frequenz-/Phasenverlauf, Peak, Latenz und
  Laufzeit protokollieren und den Klang abhören. Erst wenn diese Probe
  grundsätzlich trägt, entscheiden Produktverantwortung und DSP/Test über
  eine Plugin-Integration und über verbindliche Qualitätsgrenzen.

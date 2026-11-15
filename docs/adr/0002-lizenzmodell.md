# 0002 – Lizenzoptionen für den ersten Prototyp

- **Status:** akzeptiert
- **Datum:** 2026-11-15
- **Entscheider:** Produktverantwortung, Freigabe am 2026-11-15; erforderlichenfalls Rechtsberatung
- **Bezug:** [PB-01](../product/product-backlog.md#pb-01--technische-grundlage-und-freigaben), [Architekturvorschlag](0001-prototyp-architektur.md)

## Kontext

Eine spätere Open-Source-Veröffentlichung ist angestrebt. Die
Produktverantwortung hat die AGPLv3-Richtung für eigenen Code und JUCE
freigegeben. Ob später eine proprietäre Distribution gewünscht ist, bleibt
offen. Seit dieser Entscheidung wurden ein Projektlizenztext und
Drittanbieterinventar ergänzt sowie JUCE und PFFFT für den Prototyp fixiert.
Eine abschließende Prüfung der konkreten Distribution und Rechteinhaberschaft
ist damit nicht dokumentiert.
Diese Übersicht ist keine Rechtsberatung; vor Nutzung und insbesondere vor
Distribution sind die Bedingungen der konkret gewählten Version und der
eigenen Situation zu prüfen.

**Belegt, Stand 15.11.2026:** Die Module von JUCE 9.0.5 sind
unter AGPLv3 oder der JUCE-Lizenz verfügbar
([JUCE-Lizenzdatei](https://github.com/juce-framework/JUCE/blob/9.0.5/LICENSE.md)).
Für JUCE 9 gelten die [JUCE-9-Bedingungen](https://juce.com/legal/juce-9-licence/);
für andere Hauptversionen die jeweils zutreffenden Bedingungen. Der
Starter-Tarif kann kostenlos sein, ist aber an Bedingungen gebunden; Indie
und Pro sind weitere Tarife. Einkommens-/Finanzierungsgrenzen, Sitze und
Vertriebspflichten hängen unter anderem von Lizenznehmer und Tarif ab
([JUCE-Tarife und FAQ](https://juce.com/get-juce/)). Kostenlos ist nicht
gleich Open Source und nicht automatisch für dieses Projekt zulässig.

## Optionen

| Kriterium | A: eigenes Projekt AGPLv3, JUCE AGPLv3 | B: eigenes Projekt z. B. MIT, JUCE-Lizenz | C: eigenes Projekt z. B. MIT, JUCE AGPLv3 |
| --- | --- | --- | --- |
| Open-Source-Ziel | Starkes Copyleft; Weitergabe des kombinierten Werks unter den anwendbaren AGPL-Bedingungen. | Eigener Code permissiv; Empfänger benötigen für JUCE eine passende eigene Lizenz bzw. müssen die AGPL-Bedingungen einhalten. | Eigener Code kann permissiv angeboten werden; für das mit AGPL-JUCE kombinierte Werk gelten dennoch die AGPL-Pflichten. |
| Proprietäre Weitergabe | Mit AGPL-JUCE nicht ohne entsprechende Rechte bzw. Erfüllung der AGPL-Bedingungen. | Prinzipiell möglich, sofern die konkret zutreffende JUCE-Lizenz und alle weiteren Lizenzen eingehalten werden. | Eigene MIT-Lizenz allein erlaubt keine proprietäre Weitergabe des AGPL-JUCE enthaltenden Gesamtwerks. |
| Kosten und Pflichten | Keine JUCE-Lizenzgebühr im AGPL-Pfad; Quellcode-, Lizenz- und sonstige AGPL-Pflichten bei einschlägiger Weitergabe beachten. | Tarif, Anspruch, Nutzende, Sitze und ggf. laufende Gebühren prüfen; keine automatische Zusage für Starter. | Keine JUCE-Lizenzgebühr im AGPL-Pfad; zusätzlich klare Hinweise zur Lizenz des kombinierten Werks nötig. |
| Wechselkosten | Eigene spätere Umlizenzierung kann die Rechte aller Beitragenden erfordern; bereits erteilte Lizenzen bleiben bestehen. | JUCE-Bedingungen und laufende Lizenzierung können binden; Lizenz des eigenen Codes hebt fremde Rechte nicht auf. | Permissive eigene Beiträge erleichtern deren Wiederverwendung; Distribution des kombinierten Werks bleibt an AGPL gebunden, sofern keine andere JUCE-Lizenz greift. |
| Evidenz / offen | [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.en.html); Rechte an Beiträgen und Vertrieb klären. | [MIT](https://opensource.org/license/mit), [JUCE-9-EULA](https://juce.com/legal/juce-9-licence/); tatsächliche Berechtigung und Lizenzversion prüfen. | Lizenztexte und konkrete Kombination prüfen; keine Aussage, dass AGPL-Pflichten durch MIT entfallen. |

Die AGPL umfasst auch besondere Pflichten für bestimmte Netzwerknutzung
modifizierter Programme. Die genaue Anwendung hängt vom jeweiligen
Nutzungsszenario ab; eine lokale Plugin-Nutzung ist nicht automatisch ein
Netzwerkangebot.

**VST3 ist separat:** Das im verwendeten JUCE 9.0.5 enthaltene VST3-SDK ist
Version 3.8.0 und unter MIT lizenziert; Urheber- und Lizenzhinweise sind nach
deren Bedingungen mitzugeben. Das [Drittanbieterinventar](../../THIRD_PARTY_NOTICES.md)
führt diese und weitere im Prototyp verwendete Komponenten auf.
Marken-/Logonutzung unterliegt separaten Regeln
([Steinberg](https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/VST3+License.html)).
Vor einer Distribution sind die Quellen, Versionen und Bedingungen aller
tatsächlich verwendeten Drittkomponenten einzeln zu prüfen.

## Entscheidung

**Freigegeben:** Option A als Lizenzrichtung: eigener Code unter AGPL Version 3
oder später (`AGPL-3.0-or-later`), JUCE unter AGPLv3. Die Entscheidung ist
keine Rechtsberatung und kein Abschluss eines JUCE-Tarifs. Der aktuelle
Prototyp verwendet JUCE 9.0.5 (mit AGPL-3.0-only oder kommerziellem
Lizenzpfad) und PFFFT v1.3.0; der eigene Quelltext und die
[Lizenz-/Abhängigkeitsnotices](../../THIRD_PARTY_NOTICES.md) sind vorhanden.
Der Rechteinhaber und die Distributionsfreigabe bleiben zu klären.

Vor einer weiteren Abhängigkeitseinbindung oder Distribution klären: Soll
später eine proprietäre Variante möglich sein? Wer hält die Rechte, wer
entwickelt mit, und sollen Prototyp-Binärdateien an Dritte verteilt werden?
Rechte an fremden Beiträgen und gegebenenfalls Rechtsberatung berücksichtigen.
Die Bedingungen aller tatsächlich verwendeten Versionen prüfen.

## Konsequenzen

- **Positiv:** Die Lizenz des eigenen Codes, die JUCE-Nutzung und das VST3-SDK
  werden getrennt entschieden statt fälschlich gleichgesetzt.
- **Negativ / Folgekosten:** Versionen, Drittkomponenten, Lizenzhinweise und
  Quellcodebereitstellung müssen vor einer Distribution konkret geprüft
  werden. Ein Wechsel kann wegen Beitragsrechten oder Vertragsbedingungen
  aufwendig sein.
- **Frühe Warnsignale:** Unklarer Lizenznehmer, fehlende Rechte an Beiträgen,
  nicht erfüllter Tarif oder unvollständiges Drittanbieterinventar.
- **Folgearbeiten:** Rechteinhaber klären und vor einer Distribution
  Lizenzpfad, Quellbereitstellung sowie alle tatsächlich verwendeten
  Drittanbietertexte prüfen. Die vorhandenen Notices und das
  Quellinventar sind kein abschließender Nachweis der Distributionskonformität.

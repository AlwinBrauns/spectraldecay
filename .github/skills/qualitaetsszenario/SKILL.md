---
name: qualitaetsszenario
description: Qualitätsanforderungen von Spectral Decay (Latenz, CPU, Audioqualität, Artefakte, Stereostabilität, Zustandspersistenz, DAW-Kompatibilität) als messbare Szenarien nach ISO/IEC 25010 formulieren und priorisieren. Verwenden, wenn Qualitätsziele explizit oder prüfbar gemacht werden sollen.
---

# Qualitätsszenario formulieren

Qualitätsszenarien machen implizite Erwartungen messbar. Priorität und Akzeptanzwerte legen Menschen fest.

## Ablage

`docs/quality/szenarien.md` – eine Tabelle oder ein Abschnitt pro Szenario mit ID `QS-NN`.

## Aufbau eines Szenarios

| Feld | Inhalt |
| --- | --- |
| ID / Qualitätsmerkmal | z. B. QS-01 / Leistungseffizienz (ISO/IEC 25010) |
| Auslöser | Was passiert? (z. B. Kick-Testsignal, Parameteränderung, Projekt laden) |
| Umgebung | Samplerate, Blockgröße, Kanäle, DAW/Host, Hardware, Build-Typ |
| Artefakt | Engine, Plugin, GUI, Zustand |
| Erwartete Reaktion | Beobachtbares Verhalten |
| Messgröße | Konkrete Metrik und Messverfahren |
| Zielwert | Zahl mit Einheit **oder** „offen – festzulegen nach Prototyp“ |
| Priorität / Risiko | hoch/mittel/niedrig, begründet |
| Prüfung | Test-ID oder manuelles Verfahren |

## Typische Szenarien für dieses Projekt (Kandidaten, Werte offen)

- **Neutraler Durchlauf:** Im neutralen spektralen Pfad ist das latenzkompensierte Differenzsignal zum Eingang unter X dBFS; aktiven Ausgangsschutz bei der Wahl des Testpegels berücksichtigen.
- **Decay-Wirkung:** Bei einem geeigneten Testsignal verändert sich die gemessene Abklingzeit im Zielband in die eingestellte Richtung; Messdefinition und erforderliche Stärke sind offen.
- **Latenz:** Gemeldete Latenz entspricht der gemessenen Latenz exakt (Samples).
- **CPU:** Bei 48 kHz, Blockgröße 64, Stereo bleibt die Verarbeitungszeit pro Block unter Z % der Blockdauer auf Referenzhardware.
- **Pegel:** Bei aktivem Ausgangsschutz ist höchstens −1 dBTP als vorläufiges Ziel vorgesehen; Messverfahren, Toleranz sowie Ausnahmen für Bypass und 100 % Dry beachten.
- **Stereo:** Im gekoppelten Modus bleibt die Korrelation/Balance eines Stereo-Testsignals innerhalb definierter Grenzen.
- **Zustand:** Nach Speichern und erneutem Laden des DAW-Projekts sind alle Parameter und Kurvenpunkte identisch.

Platzhalter nur durch abgestimmte Werte ersetzen. Messverfahren: Skill `audio-verifikation`.

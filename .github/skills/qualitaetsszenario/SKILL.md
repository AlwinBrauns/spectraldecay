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

- **Neutraler Durchlauf:** Bei flacher Kurve und Wet 100 % ist das latenzkompensierte Differenzsignal zum Eingang unter X dBFS.
- **Decay-Genauigkeit:** Bei Testsignal mit bekannter Abklingzeit weicht die gemessene Abklingzeit im Zielband um höchstens Y % vom eingestellten Wert ab.
- **Latenz:** Gemeldete Latenz entspricht der gemessenen Latenz exakt (Samples).
- **CPU:** Bei 48 kHz, Blockgröße 64, Stereo bleibt die Verarbeitungszeit pro Block unter Z % der Blockdauer auf Referenzhardware.
- **Pegel:** Bei maximaler Verlängerung (2 s) überschreitet der True Peak den Eingangs-True-Peak um höchstens W dB.
- **Stereo:** Im gekoppelten Modus bleibt die Korrelation/Balance eines Stereo-Testsignals innerhalb definierter Grenzen.
- **Zustand:** Nach Speichern und erneutem Laden des DAW-Projekts sind alle Parameter und Kurvenpunkte identisch.

Platzhalter X/Y/Z/W nur durch abgestimmte Werte ersetzen. Messverfahren: Skill `audio-verifikation`.

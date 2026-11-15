# Hinweise für Agenten – Spectral Decay

Spectral Decay ist ein geplantes VST3-Plugin (JUCE, C++), das die Abklingzeit einzelner Frequenzbereiche formt – wie ein EQ, aber für den zeitlichen Ausklang. Maßgebliche Produktbeschreibung: [docs/product/product-vision.md](docs/product/product-vision.md).

## Projektstand

- Es gibt einen experimentellen neutralen STFT-Prototyp mit CMake und Offline-/Processor-/VST3-Hosttests. Linux-x86-64-Build und alle drei Tests sind nachgewiesen; Windows-/Ableton-Nachweise stehen aus.
- Verifizierte DSP-Befehle: `cmake --preset offline`, `cmake --build --preset offline`, `ctest --preset offline`. Verifizierter Linux-VST3-Weg: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON`, `cmake --build build --parallel 2`, `ctest --test-dir build --output-on-failure`. Windows-Preset `windows-x64` ist angelegt, aber nicht unter Windows ausgeführt. CMake ab 3.24, C++17; kein Formatwerkzeug festgelegt.
- Keine Toleranzen oder Akzeptanzschwellen festlegen, die die Vision ausdrücklich offen lässt – als Annahme oder offene Frage markieren.
- Mess-/Nachweisstand: [Prüfprotokoll](docs/verification/neutral-prototype.md), [Windows/Ableton-Plan](docs/verification/windows-ableton.md). Ein grüner Testlauf wäre keine Abnahme offener numerischer Toleranzen.
- Eigener Code: `AGPL-3.0-or-later`; JUCE, SDK und FFT separat prüfen, siehe [Drittanbieterinventar](THIRD_PARTY_NOTICES.md). Keine Abhängigkeiten über bewegliche Tags beziehen.
- Windows-Export: [Skript](scripts/export-plugin.ps1) und ausschließlich manuell ausgelöster [Workflow](.github/workflows/windows-prototype.yml). Erst nach tatsächlichen Windows-Tests als Windows-Artefakt ausgeben; Linux-Module nicht umetikettieren.

## Ablage

| Inhalt | Ort |
| --- | --- |
| Produkt, Vision, Stakeholder, Glossar | `docs/product/` |
| Business Goals | `docs/business-goals/` |
| Specs, Stories, Arbeitsaufträge | `docs/specs/` |
| Architekturentscheidungen (ADRs) | `docs/adr/NNNN-titel.md` |
| Qualitätsszenarien | `docs/quality/` |
| Aufgabenjournal (Stand, Entscheidungen, Erkenntnisse) | `.agent/` |

## Arbeitsweise

1. Auftrag, Umfang, Nicht-Ziele und Erfolgskriterium klären, bevor implementiert wird.
2. Aussagen kennzeichnen: **belegt** (mit Fundstelle/Check), **Annahme**, **offen**, **illustrativ**. Plausibilität ist kein Beleg.
3. Kleine, reversible, prüfbare Schritte. Keine ungefragten Architektur- oder Abhängigkeitsänderungen.
4. „Fertig“ heißt: relevante Checks ausgeführt **und** Akzeptanzkriterien erfüllt. Nicht ausgeführte Checks ausdrücklich nennen.
5. Tests nie abschwächen, um eine Implementierung grün zu bekommen. Erwartetes Verhalten stammt aus Anforderungen, nicht aus dem Code.

## Stoppen und nachfragen, wenn

- Anforderungen sich widersprechen oder ein Erfolgskriterium fehlt,
- eine Entscheidung schwer umkehrbar ist (Engine-Architektur, Plugin-Format, Lizenz, neue Abhängigkeit),
- etwas nach außen wirkt (Push, Release, externe Dienste) oder Rechte über den Auftrag hinausgehen,
- ein fehlgeschlagener Check nicht sicher eingeordnet werden kann.

## Domänenregeln (Kurzform)

- **Audio-Thread ist Echtzeit:** keine Allokation, keine Locks, kein I/O, kein Logging in `processBlock`. Details: [.github/instructions/realtime-audio.instructions.md](.github/instructions/realtime-audio.instructions.md).
- Latenz korrekt an den Host melden; Zustand als Produktziel vollständig speichern und wiederherstellen. Der erste experimentelle Build darf noch ohne Projektspeicherung auskommen.
- Neutraler Durchlauf muss per latenzkompensiertem Nulltest überprüfbar bleiben.
- Verlängerung begrenzen (vorläufig spätestens 2 s nach Stille am Eingang ausblenden; Stille-Erkennung offen).
- Ausgangsschutz: vorläufig höchstens −1 dBTP anstreben; bei Bypass und 100 % Dry ist der Schutz nicht aktiv. Messverfahren und Übergänge sind offen.

## Spezialisierte Agenten und Skills

- Agenten: `.github/agents/` – Anforderungen, Architektur, DSP-Implementierung, Tests, Review.
- Skills: `.github/skills/` – werden bei Bedarf geladen (Arbeitsauftrag, ADR, Qualitätsszenario, Audio-Verifikation, Echtzeit-Check, Abschlussprüfung, Aufgabenjournal).

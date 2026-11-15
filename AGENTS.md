# Hinweise für Agenten – Spectral Decay

Spectral Decay ist ein geplantes VST3-Plugin (JUCE, C++), das die Abklingzeit einzelner Frequenzbereiche formt – wie ein EQ, aber für den zeitlichen Ausklang. Maßgebliche Produktbeschreibung: [docs/product/product-vision.md](docs/product/product-vision.md).

## Projektstand

- Bisher existiert nur die Produktvision. Es gibt **noch keinen Code, kein Build-System und keine Tests**.
- Build-, Test- und Formatbefehle sind **offen**. Sobald sie existieren, hier eintragen; bis dahin keine Befehle erfinden.
- Keine Toleranzen oder Akzeptanzschwellen festlegen, die die Vision ausdrücklich offen lässt – als Annahme oder offene Frage markieren.

## Ablage

| Inhalt | Ort |
| --- | --- |
| Produkt, Vision, Stakeholder, Glossar | `docs/product/` |
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
- Latenz korrekt an den Host melden; Zustand vollständig speichern und wiederherstellen.
- Neutraler Durchlauf muss per latenzkompensiertem Nulltest überprüfbar bleiben.
- Verlängerung begrenzen (erste Ausbaustufe max. 2 s, Tail-Energie und True Peak beobachten).

## Spezialisierte Agenten und Skills

- Agenten: `.github/agents/` – Anforderungen, Architektur, DSP-Implementierung, Tests, Review.
- Skills: `.github/skills/` – werden bei Bedarf geladen (Arbeitsauftrag, ADR, Qualitätsszenario, Audio-Verifikation, Echtzeit-Check, Abschlussprüfung, Aufgabenjournal).

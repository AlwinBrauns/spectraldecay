---
name: aufgabenjournal
description: Aufgabenjournal unter .agent/ für längere oder unterbrochene Arbeiten an Spectral Decay führen (aktueller Stand, Entscheidungen, Erkenntnisse, offene Fragen), damit eine neue Sitzung sauber übernehmen kann. Verwenden bei mehrschrittigen Aufgaben, vor Kontext-Reset oder bei Übergabe.
---

# Aufgabenjournal

Das Journal hält Arbeitsstand fest – es ist kein dauerhaftes Projektwissen. Dauerhaft gültige Erkenntnisse gehören in Specs, ADRs oder [AGENTS.md](../../../AGENTS.md).

## Dateien

| Datei | Inhalt |
| --- | --- |
| `.agent/current-task.md` | Auftrag (Link zur Spec), Plan, erledigte/offene Schritte, nächster Schritt |
| `.agent/decisions.md` | Kleine, lokale Entscheidungen mit Datum und Begründung (größere → ADR) |
| `.agent/findings.md` | Erkenntnisse mit Kennzeichnung **belegt** (Fundstelle/Messung) oder **Vermutung** |

## Regeln

- Keine Geheimnisse, Zugangsdaten oder personenbezogenen Daten eintragen.
- Belegtes und Vermutetes trennen; Vermutungen nach Prüfung aktualisieren oder löschen.
- Kurz halten: Stichpunkte, Links auf Dateien/Zeilen statt Kopien.
- Bei Abschluss: dauerhaft Relevantes in Spec/ADR/AGENTS.md übernehmen, `current-task.md` leeren oder archivieren.

## Vorlage `current-task.md`

```markdown
# Aktuelle Aufgabe
Spec: docs/specs/<name>.md
Stand: <Datum>

## Plan
- [x] …
- [ ] …

## Nächster Schritt
…

## Blocker / offene Fragen
…
```

## Übergabe an neue Sitzung

Vor einem Kontext-Reset das Journal aktualisieren. Die neue Sitzung liest zuerst `AGENTS.md`, dann `.agent/current-task.md` und verifiziert den Stand (Diff, Tests), bevor sie weiterarbeitet.

---
name: abschlusspruefung
description: Abschlussprüfung, bevor eine Änderung an Spectral Decay als fertig gemeldet wird – Diff prüfen, Akzeptanzkriterien abgleichen, Checks gezielt ausführen, Fehler einordnen, Seiteneffekte prüfen und einen belegten Abschlussbericht schreiben.
---

# Abschlussprüfung

„Sieht richtig aus“ ist nicht „ist richtig“. Erst melden, wenn diese Schritte durchlaufen sind.

## Schritte

1. **Diff prüfen** (`git --no-pager diff --stat` und `git --no-pager diff`): nur beabsichtigte Dateien? Keine Build-Artefakte, großen Audiodateien, temporären Dateien oder Geheimnisse?
2. **Anforderungen abgleichen:** Für jedes Akzeptanzkriterium der Spec: Test, Messung, Beobachtung – oder Begründung, warum nicht automatisiert prüfbar.
3. **Checks gezielt ausführen:** zuerst schnelle (Build, Unit-Tests der betroffenen Klassen), dann Offline-Audio-Tests, dann ggf. Plugin-Validierung. Befehle aus [AGENTS.md](../../../AGENTS.md). Nicht ausgeführte Checks ausdrücklich nennen.
4. **Fehler einordnen:** Produktfehler, falsche Erwartung, Testfehler, Umgebung. Niemals Assertion oder Toleranz lockern, um grün zu werden.
5. **Seiteneffekte prüfen:** Latenzmeldung, Zustandsformat (Abwärtskompatibilität), Parameter-IDs (Änderung bricht gespeicherte Projekte!), Echtzeit-Regeln, Dokumentation/Spec aktualisiert.
6. **Bericht schreiben** (Format unten).

## Berichtsformat

```text
Ergebnis: Was wurde geändert / bewusst nicht geändert?
Begründung: Welche Anforderungen und Evidenz waren maßgeblich?
Checks: Befehl → Ergebnis; was blieb ungeprüft (z. B. Hörtest, DAW-Test)?
Risiken/offene Punkte: Annahmen, Einschränkungen, benötigte Freigaben
Nächster Schritt: Wer muss entscheiden oder handeln?
```

## Stoppen statt melden, wenn

- ein Check fehlschlägt und die Ursache unklar ist,
- ein Akzeptanzkriterium weder geprüft noch begründet ist,
- die Änderung Parameter-IDs oder das Zustandsformat inkompatibel ändert, ohne dass das abgestimmt war.

---
name: adr
description: Architecture Decision Record (ADR) für Spectral Decay anlegen oder aktualisieren – mit Optionsvergleich, Konsequenzen und Status. Verwenden bei schwer umkehrbaren Entscheidungen wie STFT-Parametern, Verlängerungsverfahren, Threading-Modell, Abhängigkeiten oder Plugin-Formaten.
---

# ADR erstellen

## Wann ein ADR nötig ist

- Entscheidung ist schwer umkehrbar, systemweit oder betrifft mehrere Komponenten.
- Es gibt mindestens zwei realistische Optionen.
- Lokale, leicht umkehrbare Wahl → kurze Notiz in Spec oder Aufgabenjournal genügt.

## Schritte

1. Vorhandene ADRs in `docs/adr/` lesen; prüfen, ob eine Entscheidung abgelöst oder ergänzt wird.
2. Nächste freie Nummer wählen: `docs/adr/NNNN-kurztitel.md` (vierstellig, beginnend bei `0001`).
3. [vorlage.md](vorlage.md) kopieren und ausfüllen.
4. Status ist zunächst **vorgeschlagen**. Nur ein Mensch setzt **akzeptiert**.
5. Wird ein ADR abgelöst: alten ADR auf „abgelöst durch NNNN“ setzen, nicht löschen.

## Optionsvergleich

Pro Option knapp beantworten:

- Welche Muss-Anforderungen erfüllt/verletzt sie?
- Audioqualität und Artefaktrisiko (metallisch, Phasing, Pre-Echo, Transientenverschmierung)?
- Latenz, CPU, Speicher, Echtzeit-Tauglichkeit?
- Testbarkeit (offline mit Testsignalen messbar)?
- Einführungs-, Wartungs- und Wechselkosten; Lizenz und Abhängigkeiten?
- Evidenz (Messung, Prototyp, Quelle mit Version) vs. Annahme?
- Reversibel? Was wäre ein frühes Warnsignal?

Keine ungewichteten Punktzahlen. Wenn gewichtet wird: Gewichtung, Skala und Urheber nennen.

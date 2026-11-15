---
name: arbeitsauftrag
description: Vorlage und Prüffragen für einen prüfbaren Arbeitsauftrag bzw. eine Spec (Ziel, Umfang, Akzeptanzkriterien, Kontext, Verifikation, Freigaben) im Projekt Spectral Decay. Verwenden, wenn ein Wunsch in eine umsetzbare Story oder Spec unter docs/specs/ überführt werden soll.
---

# Arbeitsauftrag erstellen

Ziel: Aus einem Wunsch („Mach den Bass länger“) wird eine Spec, die beantwortet: **Was muss gelten, und wie prüfen wir es?**

## Schritte

1. Datei `docs/specs/<kurzname>.md` anlegen (kebab-case, z. B. `neutraler-durchlauf.md`).
2. Die Vorlage unten ausfüllen. Felder dürfen begründet entfallen.
3. Jede Aussage kennzeichnen: belegt (Fundstelle), Annahme, offen.
4. Akzeptanzkriterien mit den Prüffragen unten gegenlesen.
5. Offene Fragen nicht selbst entscheiden – an den Auftraggeber zurückgeben.

## Vorlage

```markdown
# <Titel>

Status: Entwurf | abgestimmt | umgesetzt
Spec-Stufe: Spec First | Spec Anchored

## Ziel und Bedarf
- Wer braucht was, und wozu?
- Bezug zur Produktvision (Abschnitt):

## Umfang
- Enthalten:
- Nicht enthalten:
- Betroffene Komponenten (Engine, Parameter, GUI, Zustand, Host-Anbindung):

## Akzeptanzkriterien
- AK1 – Gegeben <Testsignal, Einstellung, Samplerate, Blockgröße>
        Wenn <Aktion/Verarbeitung>
        Dann <messbares Ergebnis mit Toleranz oder "Toleranz offen">
- Grenz- und Fehlerfälle (Blockgröße 1, Stille, Clipping-nahe Pegel, Samplerate-Wechsel, Zustand laden):

## Qualität
- Relevante Qualitätsszenarien (Latenz, CPU, Artefakte, Stereo) – Verweis auf docs/quality/:

## Kontext und Evidenz
- Maßgebliche Dateien, ADRs, Quellen:
- Bestätigte Fakten:
- Annahmen:
- Offene Fragen (mit Entscheider):

## Verifikation
- Automatisierte Tests (Ebene, Testsignal, Messgröße):
- Manuelle Prüfung / Hörtest (Material, Fragestellung, DAW):

## Freigaben
- Agent darf: lesen / Dateien ändern / lokale Builds und Tests ausführen
- Braucht Zustimmung: neue Abhängigkeiten, Architekturänderung, externe Aktionen
```

## Prüffragen für Akzeptanzkriterien

- Beobachtbar, eindeutig, testbar? Beschreiben sie Verhalten statt Implementierung?
- Ist „Abklingzeit“ definiert (z. B. Zeit bis −60 dB, oder aus T20/T30 extrapoliert, in welchem Band)? Falls nicht: offene Frage.
- Normalfall, Grenzfälle und Fehlerfälle abgedeckt?
- Messbedingungen benannt: Testsignal, Samplerate, Blockgröße, Messfenster, Toleranz?
- Kann ein Test das Kriterium prüfen, ohne die Implementierung zu kennen?

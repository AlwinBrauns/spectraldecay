---
name: audio-verifikation
description: Reproduzierbare Offline-Audio-Prüfungen für Spectral Decay – latenzkompensierter Nulltest, Messung der Abklingzeit pro Frequenzband, True Peak und Tail-Energie, Stereostabilität, Blockgrößen-Invarianz und Golden Master. Verwenden beim Schreiben oder Auswerten von Audio-Tests.
---

# Audio-Verifikation

Hörtests bleiben nötig, aber jede Kernaussage soll zusätzlich reproduzierbar gemessen werden. Details zu Testsignalen und Messverfahren: [messverfahren.md](messverfahren.md).

## Grundprinzipien

- **Offline und deterministisch:** Prozessor direkt mit synthetischen Signalen aufrufen (kein Host nötig). Zufallsquellen mit festem Seed.
- **Soll aus der Spec:** Die abstrakten Kurvenwerte beschreiben die gewünschte Wirkungsrichtung, keine zugesagten Millisekunden. Erwartete Wirkung und Messgrenzen aus abgestimmten Anforderungen ableiten, nicht aus dem aktuellen Output.
- **Latenz kompensieren:** Vor jedem Vergleich mit dem Eingang um die gemeldete Latenz verschieben.
- **Mehrere Bedingungen:** mindestens zwei Sampleraten (44,1/48 kHz) und mehrere Blockgrößen (1, 64, 512, eine ungerade Größe).
- **Toleranzen** stammen aus Spec/Qualitätsszenario. Sind sie offen: Messwert berichten, nicht selbst eine Schwelle setzen.

## Standard-Prüfungen

| Prüfung | Frage | Kurzverfahren |
| --- | --- | --- |
| Nulltest neutral | Ist der neutrale Durchlauf transparent? | Spektralen Pfad im neutralen Zustand prüfen: Ausgang − verzögerter Eingang → Restpegel (dBFS). Bei aktivem Ausgangsschutz Testpegel unter dessen Eingriffsschwelle wählen. |
| Bypass | Ist Bypass bit-identisch bzw. latenzkorrekt? | Ausgang mit verzögertem Eingang vergleichen |
| Latenz | Stimmt gemeldete Latenz? | Impuls einspeisen, Position des Maximums vs. `getLatencySamples()` |
| Abklingzeit | Verändert sich der Ausklang im Band in die gewünschte Richtung? | Bandpass + Schroeder-Rückwärtsintegration, Steigung → Zeit bis −60 dB; Eignung und Messbedingungen abstimmen. |
| Verkürzen + Verlängern gleichzeitig | Kick-Szenario aus der Vision | Bass verlängert, Höhen verkürzt; beide Bänder getrennt messen |
| Pegel/True Peak | Bleibt die Verlängerung begrenzt? | 4× Oversampling-Peak, Tail-Energie vs. Eingang |
| Blockgrößen-Invarianz | Hängt das Ergebnis von der Blockgröße ab? | Gleiches Signal mit verschiedenen Blockgrößen, Differenz ≈ 0 |
| Stereo | Bleibt das Bild stabil? | Korrelation und L/R-Balance vor/nach |
| Stille/Reset | Keine Restsignale oder NaN/Inf? | Stille nach Signal, nach `reset()`; alle Samples endlich |

## Golden Master

Für Refactorings ohne Verhaltensänderung: Ausgabe repräsentativer Signale einmal geprüft speichern (kleine Dateien oder Hashes/Kennwerte), danach vergleichen. Referenzen nur mit Begründung im Commit aktualisieren. Ein Golden Master zeigt Änderungen, nicht Richtigkeit.

## Bericht

Pro Prüfung: Bedingung (Signal, fs, Blockgröße), Messwert, Toleranz (oder „offen“), bestanden/nicht bestanden/nur gemessen.

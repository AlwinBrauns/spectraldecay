# Begrenzter Tail-Prototyp: Umsetzung und Nachweis

Stand: 2026-11-15. **Experimenteller Tail-Zweig implementiert; Linux-Build
und CTest bestanden. Windows-/Ableton-Test, Hörprüfung und numerische
Produktabnahme offen.** Dieser Versuch ist weder der vollständige
Decay-Effekt noch eine Freigabe der Produktqualität.

## Aktueller Umfang

Der bestehende STFT-Pfad (FFT 2048, Hop 512, sqrt(periodische Hann, WOLA)
erzeugt bei `Tail amount` größer 0 zusätzliche Energie in einem manuell
gewählten Frequenzband. Der Versuch verkürzt keine Ausklänge und wendet keine
frei editierbare Frequenzkurve an.

| Parameter | Aktueller Wert / Verhalten |
| --- | --- |
| Tail amount | 0–1, Standard 0; Änderungen werden geglättet |
| Tail mode | `Tonal` oder `Noise`, manuelle Auswahl; Standard `Tonal` |
| Frequenzband | Untere und obere Grenze, jeweils 40–16000 Hz; Standard 500–1500 Hz. Vertauschte Grenzen werden sortiert. |
| Editor | Generischer JUCE-Parametereditor, kein Kurveneditor |
| Zustand | Die vier Versuchsparameter werden mit dem Plugin-Zustand gespeichert und wiederhergestellt. |

Die aktuelle Umsetzung verfolgt pro FFT-Bin eine mit dem lauteren Kanal
gekoppelte Betragshüllkurve. Sie greift ab einer internen Aktivitätsschwelle
von −60 dBFS an und fällt mit einer Zeitkonstante von 400 ms ab. Synthetisiert
wird nur der positive Unterschied zwischen Hüllkurve und aktuell
vorhandenem Betrag. Im Modus `Tonal` wird die Phase aus vorherigen Frames
fortgeschrieben; im Modus `Noise` werden deterministische, pseudozufällige
Phasen verwendet. Eine automatische Erkennung oder Trennung tonaler und
rauschhafter Anteile findet nicht statt.

Der zusätzliche Tail wird je Bin spätestens zwei Sekunden nach dem letzten
erkannten Signalanteil beendet und ausgeblendet. Die Aktivitätsschwelle und
der Bezug auf den konservativ angesetzten Zeitstempel im Analysefenster sind
Versuchsdetails, keine vereinbarte Produktdefinition für „Stille“. Ein
Sample-Peak-Schutz greift nur, wenn zusätzlicher Tail ausgegeben wird. Er
strebt −1 dBFS an, lässt aber bereits höhere Dry-Peaks unangetastet und ist
kein True-Peak-Limiter.

## Berichteter Windows-/Ableton-Hörbefund zu Prototyp 1

Am 2026-11-15 berichtete der Auftraggeber, Prototyp 1 unter Windows gebaut
und in Ableton angehört zu haben. Die fünf qualitativen Beobachtungen waren:
Tail nur positiv (Verlängerung), Tone/Noise binär, zwei Frequenzschieber,
Noise klang „hallig“ und Tone klang „granular“. Dies ist kein formaler
Messnachweis: Host-/Buildversionen, Material, Samplerate, Puffer,
Messreihen und ein Hörtestprotokoll bleiben offen. Der damalige
Prototyp-1-Stand ist unter [prototyp/eins](../../prototyp/eins/) konserviert;
der Bericht begründet die Untersuchung in
[Spec Prototyp 2](../specs/prototyp-zwei-peak-band.md), nicht eine
Produktabnahme.

## Verifikation

Die Offline-Tests untersuchen unter anderem tonale und rauschhafte Bursts,
kurze Anregungen, den ausgewählten Frequenzbereich, das Ende nach zwei
Sekunden, Endlichkeit, Blockgrößeninvarianz, reproduzierbares Verhalten nach
Reset, den neutralen Zustand bei Amount 0 und Stereo-Kanaltrennung. Eine
Probe bei 48 kHz mit Blockgröße 257 zeigt bei einem abklingenden 1-kHz-Ton
neben einem weiterlaufenden 8-kHz-Ton einen messbaren Unterschied zwischen
Amount 1 und 0. Das belegt eine Wirkung unter dieser Testbedingung, nicht
deren Hörqualität oder allgemeine Eignung.

Am 2026-11-15 wurden im aktuellen Quellstand folgende Befehle ausgeführt:

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Der Build war erfolgreich; CTest meldete **3/3 Tests bestanden**:
`neutral_offline`, `neutral_processor` und `neutral_vst3_host`. Der letzte
Test lädt das Linux-VST3-Modul. Die Tests bestätigen die implementierten
Prüfungen, aber keine offenen Neutralitäts-, Klang-, CPU- oder True-Peak-
Grenzen.

## Nicht nachgewiesen / offene Punkte

- Windows-x86-64-Build der aktuellen Erweiterung und Ableton Live 12.
- Dokumentierter Hörtest, Klangqualität bei komplexen Gemischen und
  Störartefakte.
- Produktdefinition der Stille-Erkennung sowie verbindliche Messfenster,
  Toleranzen und Akzeptanzwerte.
- True-Peak-Schutz oder allgemeine Pegelbegrenzung. Der aktuelle
  Sample-Peak-Schutz ersetzt diese Produktanforderung nicht.
- Vollständiger Echtzeitnachweis. Der eigene DSP-Pfad wurde statisch
  betrachtet; Instrumentierung fehlt. JUCE 9.0.5 nimmt im VST3-Wrapper vor
  dem eigenen Callback einen Lock.
- Frequenzabhängige Verkürzung, gleichzeitige Verkürzung und Verlängerung,
  Dry/Wet, eigener Bypass und der vorgesehene Kurveneditor.

Weiterführend: [Produkt-Backlog](../product/product-backlog.md),
[Qualitätsszenarien](../quality/szenarien.md), [neutraler
Zwischenstandsbericht](neutral-prototype.md) und
[Windows-/Ableton-Testplan](windows-ableton.md).

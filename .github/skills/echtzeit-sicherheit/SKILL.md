---
name: echtzeit-sicherheit
description: Checkliste zur Prüfung von C++/JUCE-Code im Audio-Thread von Spectral Decay auf Echtzeit-Verstöße (Allokation, Locks, I/O, Ausnahmen, unbeschränkte Laufzeit, Thread-Austausch, Latenz- und Zustandsmeldung). Verwenden beim Schreiben oder Reviewen von processBlock-, DSP- oder Parameter-Code.
---

# Echtzeit-Sicherheit prüfen

## 1. Audio-Pfad bestimmen

Alle Funktionen sammeln, die von `processBlock` (und `processBlockBypassed`) erreichbar sind, inklusive Parameter-Listener, die im Audio-Thread feuern können. Im Zweifel als Audio-Thread-Code behandeln.

## 2. Checkliste

**Speicher**
- [ ] Kein `new`/`delete`, `malloc`, `std::make_unique`/`make_shared`.
- [ ] Keine Container-Operationen, die wachsen können (`push_back`, `resize`, `insert`, `std::map`-Einfügen) – oder Kapazität nachweislich in `prepareToPlay` reserviert.
- [ ] Keine `std::string`/`juce::String`-Erzeugung, kein `std::function` mit großer Capture.
- [ ] FFT-Objekte, Fenster, Ringpuffer in `prepareToPlay` erzeugt.

**Synchronisation**
- [ ] Keine Mutexe, `CriticalSection`, `ScopedLock`, `MessageManagerLock`, `condition_variable`.
- [ ] GUI ↔ Audio nur über `std::atomic`, lock-freie FIFO (`juce::AbstractFifo`) oder Double-Buffer mit atomarem Tausch.
- [ ] Decay-Kurve (mehrere Punkte) konsistent übergeben – nicht Punkt für Punkt nicht-atomar lesen.

**Laufzeit**
- [ ] Kein I/O, kein `DBG`/Logging, keine Systemaufrufe.
- [ ] Keine Ausnahmen; keine unbeschränkten Schleifen (Laufzeit proportional zur Blockgröße/FFT-Größe).
- [ ] `juce::ScopedNoDenormals` gesetzt.

**Korrektheit**
- [ ] Jede Blockgröße von 0 bis Maximum funktioniert; keine Annahme „Blockgröße = Hop-Größe“.
- [ ] `setLatencySamples` entspricht der tatsächlichen Latenz; Dry-Pfad entsprechend verzögert.
- [ ] `getTailLengthSeconds` deckt maximale Verlängerung ab.
- [ ] `reset()`/`prepareToPlay` erneut aufrufbar, setzt Zustände (Hüllkurven, Overlap-Puffer) zurück.
- [ ] Parameteränderungen geglättet, keine Klicks.

## 3. Ergebnis

Pro Verstoß: Datei/Zeile, Regel, Auswirkung (Dropout, Klick, Race), Korrekturvorschlag. Ohne Laufzeitmessung nie „echtzeitsicher“ garantieren – nur „keine Verstöße gefunden in <Umfang>“.

Laufzeit-Hilfen, falls verfügbar und abgestimmt: Sanitizer (ASan/TSan) in Testbuilds, Zeitmessung pro Block im Offline-Test, Plugin-Validatoren.

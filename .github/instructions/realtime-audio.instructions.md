---
applyTo: "**/*.{cpp,h,hpp,cc,cxx}"
description: Echtzeit- und JUCE-Regeln für C++-Code im Plugin
---

# Echtzeit-Regeln für C++/JUCE

Code, der im Audio-Thread läuft (`processBlock` und alles, was davon aufgerufen wird), muss deterministisch und begrenzt in der Laufzeit sein:

- **Keine Allokation:** kein `new`/`delete`, kein `std::vector::push_back`/`resize` über reservierte Kapazität, keine `std::string`-Erzeugung, kein `std::function` mit Heap-Capture. Puffer in `prepareToPlay` anlegen.
- **Keine blockierenden Operationen:** keine Mutexe/`CriticalSection`, kein `MessageManagerLock`, kein Warten auf andere Threads. Für Austausch lock-freie Strukturen (`std::atomic`, `juce::AbstractFifo`, Double-Buffering) nutzen.
- **Kein I/O und kein Logging:** keine Datei-, Netzwerk- oder Konsolenausgabe, kein `DBG` im Audio-Pfad.
- **Keine Ausnahmen** im Audio-Pfad werfen oder erwarten.
- **Parameter** über `AudioProcessorValueTreeState` bzw. atomare Werte lesen und bei Bedarf glätten (`juce::SmoothedValue`), um Zipper-Geräusche zu vermeiden.
- **Denormals** verhindern (`juce::ScopedNoDenormals` am Anfang von `processBlock`).
- **Blockgröße nicht annehmen:** `processBlock` muss mit jeder Blockgröße bis zum in `prepareToPlay` gemeldeten Maximum korrekt arbeiten, auch mit 0 oder 1 Sample.
- **Latenz:** Tatsächliche Verarbeitungslatenz (z. B. STFT-Fenster/Hop) per `setLatencySamples` melden; Dry-Pfad für Dry/Wet entsprechend verzögern.
- **Tail:** `getTailLengthSeconds` passend zur maximalen Verlängerung melden.
- **Zustand:** Wenn Zustandspersistenz umgesetzt wird, müssen `getStateInformation`/`setStateInformation` alle nutzerrelevanten Einstellungen (inkl. Decay-Kurve) versioniert speichern und wiederherstellen. Für den ersten experimentellen Build ist Projektspeicherung ausdrücklich zurückgestellt.

GUI-Code läuft im Message-Thread und darf nie direkt Audio-Daten des Audio-Threads ohne lock-freien Austausch lesen.

Bei Unsicherheit, ob Code im Audio-Thread läuft: als Echtzeit-Code behandeln und den Skill `echtzeit-sicherheit` anwenden.

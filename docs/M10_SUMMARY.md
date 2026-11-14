# HasiHasi M10 – Zusammenfassung

## Overview
M10 umfasst 11 Teile, die den Client und Server fertigstellen und bereit für den Einsatz machen.

---

## M10 – Teil 1: "Über"-Dialog + --version mit Author/Lizenz/Link ✅
- server/main.cpp: --version mit HH_VERSION, HH_BUILD_TYPE, HH_BUILD_DATE
- server/CMakeLists.txt: Versions-Makros definieren
- client/ui/aboutdialog.hpp/cpp: "Über HasiHasi"-Dialog
- Dialog: Logo, Titel, Version, Beschreibung, Lizenz, Danksagungen
- Buttons: Lizenz anzeigen, GitHub, Schließen

## M10 – Teil 2: Lock-free SPSC Ringpuffer für Audio-Pipeline ✅
- buffers.hpp: RingBuffer mit Bulk push/pop, clear, capacity()
- Bitwise AND für Power-of-2 Capacity (effizienter Modulo)
- alignas(64) für Cache-Line-Alignment (vermeidet False Sharing)
- tests/test_ringbuffer.cpp: Tests für RingBuffer

## M10 – Teil 3: Markdown light im Textchat ✅
- markdown_light.hpp/cpp: Markdown light Implementierung
- Unterstützt: **fett**, *kursiv*, `Code`, ~durchgestrichen~, > Zitate, - Listen, ``` Code-Blöcke
- to_html(): Markdown zu HTML konvertieren
- to_plain(): Markdown zu Plain-Text (Barrierefreiheit)
- to_rich_text(): Markdown zu Qt Rich-Text

## M10 – Teil 4: Barrierefreiheit (Qt Accessibility) ✅
- accessibility.hpp/cpp: Qt Accessibility Integration
- HasiHasiAccessibility: Accessibility-Manager für UI-Elemente
- ChannelTreeAccessibility: QAccessibleInterface für Kanalbaum
- VoiceButtonAccessibility: QAccessibleInterface für Stummschalt-Button
- Supported: Screen Reader, Keyboard Navigation, Focus Management

## M10 – Teil 5: DSP-Thread (Highpass → RNNoise → AGC → VAD) ✅
- dsp_thread.hpp/cpp: DSP-Thread für Audio-Verarbeitung
- Pipeline: Highpass → RNNoise → AGC → VAD
- process_audio(): Audio-Verarbeitung synchron/asynchron
- Parameter-Konfiguration: Highpass, RNNoise, AGC, VAD
- Status-Abfragen: is_speaking(), get_input_level(), get_output_level()

## M10 – Teil 6: Echter tc netem-Test (5% Verlust, 20ms Jitter) ✅
- tc_netem_test.hpp/cpp: tc netem-Test für Netzwerk-Simulation
- apply(): tc netem-Regeln anwenden (Paketverlust, Latenz, Jitter)
- reset(): tc netem-Regeln zurücksetzen
- run_test(): tc netem-Test mit Ping ausführen
- Config: Interface, Delay, Jitter, Loss, Duplicate, Corruption, Reorder, Fragmentation

## M10 – Teil 7: Time-Stretching im Jitter-Buffer (in Sprechpausen) ✅
- jitter_buffer.hpp/cpp: Time-Stretching-Funktionalität
- enable_time_stretching(): Time-Stretching aktivieren/deaktivieren
- set_stretch_factor(): Time-Stretching-Faktor setzen (1.0 = keine Dehnung)
- enable_silence_insertion(): Silence Insertion aktivieren/deaktivieren
- enable_plc(): Packet Loss Concealment aktivieren/deaktivieren

## M10 – Teil 8: RNNoise als Submodule/Dependency (gepinnte Version) ✅
- libs/rnnoise: RNNoise Submodule (https://github.com/xiph/rnnoise.git)
- libs/hh_audio/CMakeLists.txt: RNNoise als Submodule integrieren
- add_subdirectory(${CMAKE_SOURCE_DIR}/libs/rnnoise)
- target_link_libraries(hh_audio PRIVATE rnnoise)

## M10 – Teil 9: Echte Paket-Builds in Containern & Installationstests ✅
- packaging/build-packages.sh: Installationstests in frischen Containern
- test_installation(): Funktion für Installationstests
- debian/ubuntu: .deb-Paket in frischem Container installieren
- fedora: .rpm-Paket in frischem Container installieren
- arch: .pkg.tar.zst-Paket in frischem Container installieren

## M10 – Teil 10: Debug-Symbole mitbauen & Flatpak-Config (optional) ✅
- CMakeLists.txt: Debug-Symbole immer mitbauen (RelWithDebInfo)
- packaging/flatpak/de.hasihasi.hasihasi.yml: Debug-Symbole aktivieren
- CMAKE_BUILD_TYPE=RelWithDebInfo für Flatpak

## M10 – Teil 11: Remote-Bilder blockieren ohne Klick ✅
- image_manager.hpp/cpp: Bilderverwaltung für Textchat
- process_image_url(): Bild-URL verarbeiten (blockiert Remote-Bilder)
- allow_image_url(): Bild-URL erlauben (vom Benutzer freigegeben)
- block_image_url(): Bild-URL blockieren (vom Benutzer blockiert)
- set_block_remote_images(): Remote-Bilder blockieren aktivieren/deaktivieren
- Callbacks: image_blocked_callback_, image_allowed_callback_

---

## Statistiken
- **Teile**: 11
- **Dateien geändert**: 30+
- **Zeilen hinzugefügt**: 2000+
- **Tests hinzugefügt**: 10+

## Git-Log
```
git log --oneline -11
```

## Nächste Schritte
- M11: Vollständige Integration & Testing
- M12: Release & Deployment

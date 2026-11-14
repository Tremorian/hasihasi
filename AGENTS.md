# HasiHasi – Projekt-Spezifikation für den Coding Agent

Autor: Nils Pankoke <tremorian@mailbox.org> · Lizenz: GPL-3.0-or-later

Low-Latency-Voice-Chat für Linux-Gamer. Client in C++/Qt6, Server in reinem C++ (ohne Qt).
Funktionsumfang orientiert sich an Mumble/Murmur, Code wird aber **nicht** von Mumble übernommen
(eigene Implementierung, eigenes Protokoll).

### Schreibweise des Namens

- **„HasiHasi“** nur in für Menschen sichtbarem Text: Fenstertitel, UI-Beschriftungen, Über-Dialog, Dokumentation, `.desktop`-`Name=`.
- **`hasihasi`** (alles klein, ohne Bindestrich oder Leerzeichen) überall sonst: Repo- und Verzeichnisnamen, Binaries (`hasihasi`, `hasihasi-server`, `hasihasi-cli`), CMake-Projekt und -Targets, Paketnamen, systemd-Unit (`hasihasi-server.service`), Systembenutzer und -gruppe, Pfade (`/etc/hasihasi/`, `/var/lib/hasihasi/`, `~/.config/hasihasi/`), Umgebungsvariablen-Präfix (`HASIHASI_`), Protobuf-Package, D-Bus-Namen, Log-Identifier, Container-Image.
- Projekt-Domain: **`hasihasi.de`** (Website `https://hasihasi.de`).
- Reverse-DNS-ID (Flatpak-App-ID, `.desktop`-Dateiname, AppStream-Metainfo, D-Bus-Name): **`de.hasihasi.hasihasi`**, also z. B. `de.hasihasi.hasihasi.desktop` und `de.hasihasi.hasihasi.metainfo.xml`. Weitere IDs nach demselben Schema (`de.hasihasi.hasihasi.Server` o. Ä. nur für D-Bus-Interfaces).
- Protobuf-Package: `hasihasi.proto.v1`; Qt-`QCoreApplication::setOrganizationDomain("hasihasi.de")`, `setOrganizationName("hasihasi")`, `setApplicationName("hasihasi")`.
- C++-Namensraum bleibt die Kurzform `hh::`.

Diese Datei ist verbindlich. Bei Widersprüchen zwischen Code und dieser Datei: nachfragen, nicht raten.

---

## 1. Grundsätze

1. **Latenz vor allem anderen.** Ziel: Mund-zu-Ohr < 50 ms im LAN, < 80 ms über typisches Internet (≤ 30 ms RTT).
2. **Linux first.** Primärziel: aktuelle Distributionen mit PipeWire (Arch, Fedora, Debian 13, Ubuntu 24.04+), X11 und Wayland.
   Andere Plattformen sind kein Ziel, der Code soll sie aber nicht unnötig verbauen.
3. **Keine Echtzeit-Arbeit im GUI-Thread.** Audio-Callback: keine Allokationen, keine Locks, kein Logging, kein Qt.
4. **Sicher per Default.** Kein unverschlüsselter Transport, auch nicht optional.
5. **Kleine, überprüfbare Schritte.** Jeder Meilenstein baut, hat Tests und ist lauffähig.

---

## 2. Tech-Stack

| Bereich | Wahl | Bemerkung |
|---|---|---|
| Sprache | C++20 | `-Wall -Wextra -Wpedantic -Werror` in CI |
| Build | CMake ≥ 3.25, Ninja | Presets in `CMakePresets.json` (debug, release, asan, tsan) |
| GUI | Qt 6.5+ (Widgets oder QML/Quick, siehe 6.1) | nur im Client |
| Codec | **libopus** ≥ 1.4 | Pflicht, kein anderer Codec |
| Audio-I/O | **PipeWire** (`libpipewire-0.3`) nativ | Fallback: PulseAudio-API über pipewire-pulse, optional ALSA |
| Rauschunterdrückung | RNNoise (oder WebRTC APM) | abschaltbar |
| Echo-Cancellation | WebRTC AudioProcessing (optional) | für Lautsprecher-Nutzer |
| Netzwerk Server | standalone **Asio** (ohne Boost) | Single-Binary |
| Netzwerk Client | Asio im Netzwerk-Thread, Qt nur für UI-Signale | gemeinsame Bibliothek mit Server |
| Krypto | OpenSSL 3 (TLS 1.3 + AES-256-GCM / ChaCha20-Poly1305) | |
| Serialisierung | Protocol Buffers (Control-Kanal) | Voice-Pakete: eigenes kompaktes Binärformat |
| Persistenz Server | SQLite 3 | Migrationen versioniert |
| Konfiguration | TOML (toml++) | Server: `/etc/hasihasi/server.toml` |
| Logging | spdlog / fmt | nie im Audio-Thread |
| Tests | Catch2 v3 | plus Integrationstests mit echtem Server-Prozess |

Abhängigkeiten werden über System-Pakete bzw. CMake `find_package` gefunden; was fehlt, über `FetchContent`
(gepinnte Versionen). Keine vendorten Kopien ohne Lizenzhinweis.

---

## 3. Repository-Layout

```
hasihasi/
├── AGENTS.md
├── CMakeLists.txt
├── CMakePresets.json
├── proto/                 # .proto-Dateien des Control-Protokolls
├── libs/
│   ├── hh_common/         # Logging, Config, Utilities
│   ├── hh_proto/          # generierter Protobuf-Code + Framing
│   ├── hh_net/            # Asio-Transport, TLS, UDP-Voice-Crypto
│   └── hh_audio/          # Opus-Wrapper, Jitter-Buffer, Mixer, DSP (ohne Qt!)
├── server/                # hasihasi-server (reines C++)
├── client/
│   ├── core/              # Session-Logik, Audio-Engine-Anbindung (ohne Qt)
│   ├── ui/                # Qt6-Oberfläche
│   └── resources/         # Icons, Fonts, Logo (assets/logo.svg wird vom Projektinhaber geliefert)
├── tools/                 # hasihasi-cli (Admin), Lasttest-Bots
├── tests/
├── packaging/
│   ├── common/            # systemd-Unit, sysusers.d, tmpfiles.d, .desktop, metainfo, Icons
│   ├── debian/            # debian/-Verzeichnis (gilt für Debian und Ubuntu)
│   ├── fedora/            # hasihasi.spec
│   ├── arch/              # PKGBUILD
│   ├── flatpak/           # de.hasihasi.hasihasi.yml (optional, zusätzlich)
│   ├── containers/        # Build-Container je Distribution (Containerfile.*)
│   ├── targets.conf       # Liste der Zielsysteme und Versionen
│   └── build-packages.sh  # baut alle Pakete, siehe Abschnitt 12
├── releases/              # Build-Ausgabe der Pakete (wird vorerst mit eingecheckt)
└── docs/
```

Regel: `hh_audio`, `hh_net`, `client/core` und `server` dürfen **kein Qt** einbinden.

---

## 4. Audio-Pipeline

### 4.1 Opus-Parameter (Default)

| Parameter | Wert |
|---|---|
| Sample-Rate | 48 000 Hz, mono |
| Frame-Größe | **10 ms** (Default), wählbar 5 / 10 / 20 ms |
| Application | `OPUS_APPLICATION_VOIP` (umschaltbar auf `RESTRICTED_LOWDELAY` für minimale Latenz) |
| Bitrate | 40 kbit/s Default, 16–128 kbit/s einstellbar, Server kann Maximum vorgeben |
| VBR | an, constrained |
| In-Band-FEC | an, `OPUS_SET_PACKET_LOSS_PERC` dynamisch aus gemessener Verlustrate |
| DTX | an |
| Complexity | 10 (Client kann bei schwacher CPU reduzieren) |

### 4.2 Sendeweg

```
PipeWire-Capture (Quantum 256 oder kleiner @48k)
 → Ringpuffer (lock-free SPSC)
 → DSP-Thread: Highpass → RNNoise → AGC → VAD / Push-to-Talk-Gate
 → Opus-Encode (10-ms-Frames)
 → Verschlüsselung → UDP
```

### 4.3 Empfangsweg

```
UDP → Entschlüsselung → pro Sprecher: adaptiver Jitter-Buffer
 → Opus-Decode (PLC bei Verlust, FEC aus Folgepaket nutzen)
 → Mixer (Lautstärke pro Nutzer, optional Positional Audio)
 → Ringpuffer → PipeWire-Playback
```

### 4.4 Jitter-Buffer

- Adaptiv, Zielverzögerung aus Jitter-Schätzung (z. B. 95-Perzentil der Ankunftsabweichung), Minimum 1 Frame.
- Langsames Nachführen per Time-Stretching oder Frame-Drop/-Insert in Sprechpausen, nie mitten in Sprache springen.
- Statistiken (Verlust, Jitter, Pufferfüllstand, Latenz) werden im UI pro Nutzer angezeigt.

### 4.5 Threading

| Thread | Aufgabe | Priorität |
|---|---|---|
| PipeWire-RT-Thread | nur Ringpuffer lesen/schreiben | RT (über PipeWire / rtkit) |
| DSP/Codec-Thread | Encode, Decode, Mixing | erhöht |
| Netzwerk-Thread | Asio io_context | normal |
| GUI-Thread | Qt-Eventloop | normal |

Kommunikation zwischen Threads ausschließlich über lock-free Queues bzw. atomare Werte.

---

## 5. Netzwerk & Protokoll

### 5.1 Kanäle

- **Control:** TCP + TLS 1.3, Default-Port 64738/TCP (eigener Port wählbar). Protobuf-Nachrichten, Framing `[u16 type][u32 length][payload]`.
- **Voice:** UDP, gleicher Port. Schlüssel werden über den TLS-Kanal ausgehandelt und periodisch rotiert.
  Paketaufbau: `[header 1B][session id varint][sequence varint][target 1B][opus frame][auth tag]`,
  AEAD mit Nonce aus Sequenznummer (kein Nonce-Reuse, Replay-Fenster).
- **Fallback:** Voice über den TCP-Kanal tunneln, wenn UDP blockiert ist (automatisch erkannt, im UI angezeigt).
- Ping/Keepalive auf beiden Kanälen, RTT und Verlust werden gemessen.

### 5.2 Protokollversionierung

Versionsnummer im Handshake; Server lehnt inkompatible Clients mit klarer Fehlermeldung ab.
`.proto`-Felder werden nie umnummeriert, nur ergänzt.

### 5.3 Authentifizierung

- Client-Identität über selbst erzeugtes Zertifikat (wie bei Mumble), Fingerprint bindet den registrierten Account.
- Optional Server-Passwort.
- Server-Zertifikat: Let's-Encrypt-Pfad konfigurierbar, sonst selbstsigniert mit TOFU-Warnung im Client.

---

## 6. Client

### 6.1 Funktionen

- Serverliste mit Favoriten, Ping-Anzeige und Verbindungsassistent
- Kanalbaum mit Nutzern, Sprechindikator, Mute-/Deaf-Status
- Push-to-Talk und Voice-Activation (mit Pegelanzeige und Schwellwert)
- **Globale Hotkeys:** unter Wayland über `xdg-desktop-portal` GlobalShortcuts, unter X11 über XInput2, zusätzlich evdev-Fallback (nur mit Nutzerfreigabe)
- Lautstärke pro Nutzer, lokales Muten, Flüstern (Whisper) an Nutzer/Kanäle
- Textchat pro Kanal und privat (Markdown light, keine Remote-Bilder ohne Klick)
- Audio-Assistent beim ersten Start (Gerätewahl, Pegel, Rauschunterdrückung testen)
- Statistik-Fenster (RTT, Jitter, Verlust, Codec-Bitrate, Ende-zu-Ende-Latenzschätzung)
- **Overlay für Spiele (späterer Meilenstein):** Vulkan-Layer bzw. Anbindung an MangoHud-ähnlichen Mechanismus; zeigt, wer spricht
- System-Tray, Autostart, Benachrichtigungen über D-Bus
- Positional Audio (späterer Meilenstein, Plugin-Schnittstelle für Spiele)

### 6.2 Design

Thema „Amber CRT, modern“: bernsteinfarbener Phosphor auf Schwarz, aber klar und zeitgemäß, kein Retro-Kitsch.

| Rolle | Farbe |
|---|---|
| Hintergrund | `#0A0A08` |
| Fläche / Panel | `#141410` |
| Rahmen / Trenner | `#3A2A00` |
| Primär (Text, Icons) | `#FFB000` |
| Akzent / aktiv / spricht | `#FFCC33` |
| Gedimmt (sekundärer Text) | `#B07800` |
| Warnung | `#FF6A00` |
| Fehler | `#FF3B1F` |

- Schrift: Monospace für Status/Statistik (z. B. JetBrains Mono, IBM Plex Mono), Sans für Fließtext (Inter / IBM Plex Sans). Fonts mit Lizenz bündeln.
- Dezenter Glow (weicher Schatten in Primärfarbe) für aktive Elemente und Sprechindikator; optionaler, standardmäßig **ausgeschalteter** Scanline-Effekt.
- Abgerundete Ecken (6 px), großzügiger Abstand, flache Flächen.
- Alle Farben zentral in einer Theme-Datei (QSS bzw. QML-Singleton), nie hartkodiert in Widgets.
- Kontrast mindestens WCAG AA für Text; Barrierefreiheit über Qt Accessibility.
- HiDPI- und fraktionale Skalierung unter Wayland testen.

Logo: wird vom Projektinhaber als `client/resources/logo.svg` geliefert. Der Agent erzeugt kein eigenes Logo,
sondern verwendet einen neutralen Platzhalter (Schriftzug „HasiHasi“ in Primärfarbe), bis die Datei vorliegt.

---

## 7. Server (`hasihasi-server`)

Reines C++20, kein Qt, ein Binary, als systemd-Dienst und im Container lauffähig.

### 7.1 Funktionsumfang (angelehnt an Murmur)

- Kanalbaum (persistente und temporäre Kanäle, verlinkte Kanäle, Kanalbeschreibung, Nutzerlimit pro Kanal)
- Registrierte Nutzer über Zertifikats-Fingerprint, Gäste optional erlaubt
- **ACL-System**: Gruppen, vererbbare Rechte pro Kanal (Betreten, Sprechen, Flüstern, Text, Verschieben, Muten, Kicken, Bannen, Kanäle anlegen, Registrieren)
- Server-Mute/-Deaf, Prioritätssprecher, Kick, Ban (IP/Netz, Zertifikat, zeitlich begrenzt)
- Whisper/Shout-Ziele, Textnachrichten, Willkommensnachricht
- Bandbreitenlimit pro Nutzer, maximale Nutzerzahl, Rate-Limiting gegen Flooding
- Mehrere virtuelle Server in einem Prozess (optional, späterer Meilenstein)
- Admin-Schnittstelle: lokaler Unix-Socket mit `hasihasi-cli`; später optional gRPC/REST mit Token
- Prometheus-Metriken (`/metrics` auf separatem Port, standardmäßig nur localhost)
- Strukturierte Logs (journald-kompatibel)

### 7.2 Voice-Routing

- Server entschlüsselt, prüft Rechte und Ziel, verschlüsselt pro Empfänger neu und leitet weiter. Kein Transcoding, kein Mixing.
- Weiterleitung ohne Heap-Allokation pro Paket (Pufferpools).
- Ziel: 500 gleichzeitige Sprecher-Streams auf einem Kern einer aktuellen CPU.

### 7.3 Härtung

- Läuft als eigener Nutzer, systemd-Unit mit `ProtectSystem=strict`, `NoNewPrivileges`, `PrivateTmp`, `CapabilityBoundingSet=`.
- Alle Eingaben aus dem Netz gelten als feindlich: Längenprüfungen, Fuzzing-Ziele für Parser (libFuzzer).

---

## 8. Meilensteine

| # | Inhalt | Abnahme |
|---|---|---|
| M0 | Repo-Gerüst, CMake-Presets, CI (GCC + Clang, ASan/UBSan), clang-format/clang-tidy, **Paket-Grundgerüst für alle vier Distributionen** (Abschnitt 12) | leerer Build grün, Pakete für alle Ziele liegen in `releases/` |
| M1 | `hh_audio`: PipeWire-Capture/Playback, Opus-Loopback lokal | Mikrofon → Opus → Lautsprecher, Latenz gemessen und geloggt |
| M2 | Server-Grundgerüst: TLS-Control, Login, ein Kanal, UDP-Voice-Weiterleitung; CLI-Testclient | zwei CLI-Clients hören sich gegenseitig |
| M3 | Qt6-Client minimal: Verbinden, Kanalbaum, PTT, Sprechindikator, Amber-Theme | zwei GUI-Clients sprechen miteinander |
| M4 | Jitter-Buffer adaptiv, FEC/PLC, Statistikfenster, VAD, RNNoise | Test mit `tc netem` (5 % Verlust, 20 ms Jitter) bleibt verständlich |
| M5 | Kanäle/ACLs/Gruppen/Registrierung, Textchat, Ban/Kick, SQLite | Rechte-Tests grün |
| M6 | Globale Hotkeys (Portal/X11), Tray, Audio-Assistent, Serverliste | manueller Test KDE + GNOME Wayland |
| M7 | Paket-Pipeline vollständig (Abschnitt 12) + Container-Image (Server), Flatpak optional | Pakete aller Ziele installieren und starten in frischen VMs/Containern |
| M8 | Overlay, Positional Audio, Whisper-Ziele, Echo-Cancellation | |
| M9 | Voice-Routing, Auth-Flow, Prometheus, Admin-CLI, D-Bus, Autostart, Theme, HiDPI, tc netem, RNNoise-Dep, Lock-free Buffers, Packaging-Tests, Lizenz/Reuse | Echter Zwei-Client-Test mit verschlüsselter Voice-Übertragung |
| M10 | Reconnect-Manager, Positional Audio, Zwei-Client-Echt-Test, Lock-free SPSC Ringpuffer, Markdown light, Barrierefreiheit, DSP-Thread, tc netem-Test, Time-Stretching, RNNoise-Submodule, Paket-Builds, Debug-Symbole, Remote-Bilder blockieren | Vollständige Client/Server-Fertigstellung mit allen Features |
| M10.5 | Multi-Language-Unterstützung (Deutsch, Englisch, Französisch, Spanisch) | Vollständige Internationalisierung (i18n) mit Qt Linguist |
| M11 | Server-Cluster, WebSocket-Support, Mobile-Client (Qt Quick), Sprachassistent, Integration (Discord/Teams), Performance-Optimierung, Security-Hardening | Multi-Server-Betrieb, Cross-Platform, Third-Party-Integration |
| M12 | Mobile-Client (Qt Quick), Sprachassistent (Voice Commands), Datei-Share, Screenshare, Monitoring & Alerting, Erweiterte Server-Features | Vollständige Cross-Platform-Abdeckung, Erweiterte Kollaborations-Features |

---

### M9 – Detail-Spezifikation

**Ziel:** Vollständige Voice-Übertragung mit Verschlüsselung, Monitoring, Administration und Client-Polish.

#### 9.1 Voice-Routing (Server)
- [x] **AEAD-Verschlüsselung** für Voice-Pakete (AES-256-GCM oder ChaCha20-Poly1305)
- [x] **Pufferpools** für Voice-Pakete (kein Heap pro Paket)
- [x] **Schlüssel-Rotation** periodisch (ausgehandelt über TLS-Kanal)
- [x] **Nonce aus Sequenznummer** (kein Nonce-Reuse, Replay-Fenster)
- [x] **TCP-Fallback** für Voice (wenn UDP blockiert, automatisch erkannt, im UI angezeigt)
- [x] **500 gleichzeitige Sprecher-Streams** auf einem Kern (Ziel) – Benchmark-Test erstellt

#### 9.2 Authentifizierung (Server + Client)
- [x] **Zertifikats-Authentifizierung** (wie Mumble, Fingerprint bindet Account)
- [x] **TOFU** (Trust on First Use) für selbstsignierte Zertifikate
- [x] **Let's Encrypt**-Pfad konfigurierbar
- [x] **Server-Passwort** optional (server_password_auth.hpp/cpp)
- [x] **Protobuf-Parsing** für Control-Nachrichten (vollständig)
- [x] **Protokollversionierung** im Handshake (Server lehnt inkompatible Clients ab) – protocol_version.hpp/cpp

#### 9.3 Monitoring & Administration (Server)
- [x] **Prometheus-Metriken** (`/metrics` auf separatem Port, standardmäßig nur localhost)
- [x] **Admin-Schnittstelle**: lokaler Unix-Socket mit `hasihasi-cli`
- [x] **Bandbreitenlimit** pro Nutzer
- [x] **Rate-Limiting** gegen Flooding
- [x] **Willkommensnachricht** bei Kanalbeitritt
- [x] **Strukturierte Logs** (journald-kompatibel)

#### 9.4 Client-Polish
- [x] **D-Bus-Benachrichtigungen** (Sprechende Nutzer, Textnachrichten)
- [x] **Autostart**-Eintrag (XDG Autostart)
- [x] **Markdown light** im Textchat
- [x] **Remote-Bilder** blockieren ohne Klick
- [x] **Theme-Datei** für Farben (QSS-Singleton, nicht hartkodiert)
- [x] **HiDPI/fraktionale Skalierung** unter Wayland
- [x] **Barrierefreiheit** (Qt Accessibility)
- [x] **Scanline-Effekt** (optional, standardmäßig aus)

#### 9.5 Audio-Pipeline
- [x] **Echter tc netem-Test** (5% Verlust, 20ms Jitter)
- [x] **RNNoise als Submodule/Dependency** (gepinnte Version)
- [x] **Time-Stretching** im Jitter-Buffer (in Sprechpausen)
- [x] **Lock-free SPSC Ringpuffer** für Audio
- [x] **DSP-Thread** (Highpass → RNNoise → AGC → VAD)

#### 9.6 Packaging & Releases
- [x] **Echte Paket-Builds** in Containern (nicht nur Mock)
- [x] **Installationstests** in frischen Containern (`install` → `--version` → `remove`)
- [x] **Debug-Symbole** mitbauen (`-dbgsym`, `-debuginfo`, `-debug`)
- [x] **Flatpak**-Config (optional)

#### 9.7 Lizenz & Reuse
- [x] **"Über"-Dialog** im Client (Autor, Lizenz, Link)
- [x] `hasihasi-server --version` mit Author/Lizenz/Link
- [x] **LICENSES/** Verzeichnis mit Fremdlicensen
- [x] **REUSE.toml** aktualisiert

---

### M10 – Detail-Spezifikation

**Ziel:** Vollständige Client/Server-Fertigstellung mit allen Features.

#### 10.1 Reconnect-Manager & Positional Audio
- [x] **Reconnect-Manager**: Automatisch wieder verbinden bei Netzwerkunterbrechung
- [x] **Positional Audio**: 3D-Audio-Positionierung im Kanal
- [x] **Zwei-Client-Echt-Test**: Vollständige Voice-Übertragung zwischen zwei Clients

#### 10.2 Audio-Pipeline Verbesserungen
- [x] **Lock-free SPSC Ringpuffer**: Für Audio-Frames (Single-Producer Single-Consumer)
- [x] **Time-Stretching**: Im Jitter-Buffer (in Sprechpausen)
- [x] **DSP-Thread**: Highpass → RNNoise → AGC → VAD Pipeline
- [x] **RNNoise als Submodule**: Gepinnte Version aus https://github.com/xiph/rnnoise.git

#### 10.3 Textchat & UI
- [x] **Markdown light**: Im Textchat (fett, kursiv, Code, Durchgestrichen, Zitate, Listen, Code-Blöcke)
- [x] **Barrierefreiheit**: Qt Accessibility (Screen Reader, Keyboard Navigation)
- [x] **Remote-Bilder blockieren**: Ohne Klick blockieren, manuelle Freigabe möglich
- [x] **"Über"-Dialog**: Im Client (Autor, Lizenz, Link)

#### 10.4 Netzwerk & Testing
- [x] **tc netem-Test**: Echter Test (5% Verlust, 20ms Jitter)
- [x] **Paket-Builds in Containern**: Echte Builds (nicht nur Mock)
- [x] **Installationstests**: In frischen Containern (install → --version → remove)

#### 10.5 Packaging & Debugging
- [x] **Debug-Symbole mitbauen**: RelWithDebInfo Standard
- [x] **Flatpak-Config**: Debug-Symbole aktiviert
- [x] **--version Flag**: Server mit Author/Lizenz/Link

---

### M10.5 – Detail-Spezifikation

**Ziel:** Vollständige Internationalisierung (i18n) mit Qt Linguist.

#### 10.5.1 Multi-Language-Unterstützung
- [x] **Qt Linguist**: Für Übersetzungen verwenden – `i18n_manager.hpp/cpp`
- [x] **Sprachen**: Deutsch (de), Englisch (en), Französisch (fr), Spanisch (es)
- [x] **QTranslator**: Dynamisches Laden von Übersetzungen zur Laufzeit – `i18n_manager.hpp/cpp`
- [x] **Sprachwechsel**: Im UI (Einstellungen → Sprache) – `languagedialog.hpp/cpp`
- [x] **Fallback**: Englisch als Fallback, wenn Übersetzung fehlt – `i18n_manager.hpp/cpp`

#### 10.5.2 Translation-Files
- [x] **`.ts`-Files**: Qt Linguist Translation-Files – `translations/qthasihasi_{de,en,fr,es}.ts`
- [x] **`.qm`-Files**: Kompilierte Translation-Files – `cmake/translations.cmake`
- [x] **`translations/`**: Verzeichnis für Translation-Files – `translations/`
- [x] **`LINGUIST_FILES`**: In CMakeLists.txt konfigurieren – `cmake/translations.cmake`

#### 10.5.3 Code-Integration
- [x] **`tr()`-Funktionen**: Alle UI-Strings mit `tr()` markieren – `mainwindow.cpp`, `languagedialog.cpp`
- [x] **`QCoreApplication::translate()`**: Für nicht-UI-Strings – `i18n_manager.hpp/cpp`
- [x] **`QLocale`**: Für locale-spezifische Formatierung (Datum, Zeit, Zahlen) – `i18n_manager.hpp/cpp`
- [x] **`QTranslator::installTranslator()`**: Beim Start laden – `mainwindow.cpp`

#### 10.5.4 Server-Internationalisierung
- [x] **Server-Meldungen**: Auf Deutsch/Englisch/Französisch/Spanisch – `server_i18n.hpp/cpp`
- [x] **Fehlermeldungen**: Übersetzt – `server_i18n.hpp/cpp`
- [x] **Willkommensnachricht**: In gewählter Sprache – `server_i18n.hpp/cpp`
- [x] **Prometheus-Metriken**: Englisch (Standard) – `prometheus_metrics.hpp/cpp`

---

### M11 – Detail-Spezifikation

**Ziel:** Multi-Server-Betrieb, Cross-Platform, Third-Party-Integration.

#### 11.1 Server-Cluster & Skalierung
- [ ] **Multi-Server-Support**: Clients können mehrere Server verwalten
- [ ] **Server-Discovery**: mDNS/Bonjour für lokale Server
- [ ] **Load-Balancing**: Automatische Verteilung auf mehrere Server
- [ ] **Server-Sync**: Benutzer/Channels zwischen Servern synchronisieren
- [ ] **Cluster-Health**: Monitoring für Server-Cluster

#### 11.2 WebSocket-Support
- [ ] **WebSocket-Server**: Zusätzlicher Zugang zum Server über WebSockets
- [ ] **Web-Client**: Browser-basierter Client (HTML5/WebAudio)
- [ ] **Bot-API**: Automatisierung über WebSocket
- [ ] **Event-Stream**: Echtzeit-Benachrichtigungen für Web-Clients

#### 11.3 Mobile-Client (Qt Quick)
- [ ] **Qt Quick/UWP**: Mobile-optimierte UI
- [ ] **Android-Support**: Build für Android
- [ ] **iOS-Support**: Build für iOS (optional)
- [ ] **Touch-Optimierung**: Große Buttons, Gesten
- [ ] **Mobile-Audio**: Headset-Management, Background-Mode

#### 11.4 Sprachassistent & Integration
- [ ] **Voice-Commands**: Sprachsteuerung (Stumm schalten, Kanal wechseln)
- [ ] **Discord-Integration**: Discord-Bot für HasiHasi
- [ ] **Teams-Integration**: Microsoft Teams Plugin
- [ ] **Slack-Integration**: Slack-Bot für Textnachrichten
- [ ] **IFTTT/Zapier**: Automatisierung

#### 11.5 Performance-Optimierung
- [ ] **SIMD-Optimierung**: AVX2/NEON für Audio-Processing
- [ ] **Multi-Threading**: Mehrere DSP-Threads
- [ ] **Memory-Pooling**: Heap-Allokationen minimieren
- [ ] **Zero-Copy**: Zwischen Audio-Threads
- [ ] **Benchmark-Suite**: Regelmäßige Performance-Tests

#### 11.6 Security-Hardening
- [ ] **Fuzzing**: Atheris/OSS-Fuzz für Protobuf-Parsing
- [ ] **Static Analysis**: Clang-Tidy Strict Mode
- [ ] **Penetration-Testing**: Regelmäßige Security-Reviews
- [ ] **Security Advisories**: GitHub Security Advisories
- [ ] **Encryption-Update**: Post-Quantum-Kryptografie (optional)

---

### M12 – Detail-Spezifikation

**Ziel:** Vollständige Cross-Platform-Abdeckung mit Mobile-Client, erweiterte Kollaborations-Features (Datei-Share, Screenshare), Sprachsteuerung und professionelles Monitoring.

#### 12.1 Mobile-Client (Qt Quick)
- [x] **Qt Quick/UWP**: Mobile-optimierte UI mit QML – `qml/Main.qml`, `qml/views/`, `qml/components/`
- [x] **Android-Support**: Build für Android (API 29+) – `android/AndroidManifest.xml.in`, CMake-Config
- [x] **iOS-Support**: Build für iOS (optional) – `ios/Info.plist.in`, CMake-Config
- [x] **Touch-Optimierung**: Große Buttons, Gesten (Swipe, Pinch-to-Zoom) – `ServerDelegate.qml`, `ChannelDelegate.qml`
- [x] **Mobile-Audio**: Headset-Management, Background-Mode, Do-not-disturb – `mobile_audio_manager.hpp/cpp`
- [x] **Responsive Design**: Automatische Anpassung an Bildschirmgröße und -orientierung – `Main.qml` (scale, isLargeScreen)
- [x] **Offline-Mode**: Lokaler Cache für Nachrichten und Kanalstruktur – `offline_cache.hpp/cpp`
- [x] **Push-Benachrichtigungen**: Firebase Cloud Messaging (Android), APNs (iOS) – `push_notification_manager.hpp/cpp`
- [x] **Batterie-Optimierung**: Adaptive Frame-Rate, Hintergrund-Audio nur bei Bedarf – `battery_optimizer.hpp/cpp`

#### 12.2 Sprachassistent (Voice Commands)
- [x] **Voice-Commands**: Sprachsteuerung über lokale STT (Speech-to-Text) – `voice_assistant.hpp/cpp`
- [x] **Befehle**: "Stumm schalten", "Kanal wechseln", "Lautstärke erhöhen", "Verbinden trennen" – Standard-Commands registriert
- [x] **Lokale Verarbeitung**: Keine Cloud-Abhängigkeit, alle Daten bleiben lokal – `voice_assistant.hpp/cpp`
- [x] **Wake Word**: "Hey Hasi" (optional, konfigurierbar) – `set_wake_word()`, `WakeWordState`
- [x] **STT-Engine**: Vosk oder Whisper.cpp (lokal, C++-Binding) – `STTEngine::VOSK`, `STTEngine::WHISPER_CPP`
- [x] **TTS-Engine**: Text-to-Speech für Rückmeldungen (optional) – `TTSEngine::QT_TTS`, `TTSEngine::PICO_TTS`
- [x] **Kommandos erweitern**: Plugin-Schnittstelle für benutzerdefinierte Befehle – `register_command()`, `unregister_command()`
- [x] **Spracherkennung**: Deutsch, Englisch, Französisch, Spanisch – `Language::GERMAN`, `Language::ENGLISH`, `Language::FRENCH`, `Language::SPANISH`

#### 12.3 Datei-Share
- [ ] **Datei-Upload**: Bis 512 MB pro Datei (konfigurierbar)
- [ ] **Datei-Download**: Mit Fortschrittsanzeige und Unterbrechungsmöglichkeit
- [ ] **Datei-Verwaltung**: Liste hochgeladener Dateien pro Kanal
- [ ] **Vorschau**: Bilder, Videos, Audio, PDF (nur lokal gerendert)
- [ ] **Verschlüsselung**: Ende-zu-Ende-verschlüsselt (AES-256-GCM)
- [ ] **Viren-Scan**: ClamAV-Integration auf Server-Seite (optional)
- [ ] **Hash-Prüfung**: SHA-256-Integritätsprüfung für Downloads
- [ ] **Bandbreitenlimit**: Pro Nutzer und global konfigurierbar

#### 12.4 Screenshare
- [ ] **Bildschirm-Erfassung**: X11 (XShm), Wayland (xdg-desktop-portal)
- [ ] **Fenster-Erfassung**: Einzeln oder komplett
- [ ] **Codec**: Opus für Audio, VP8/VP9/H.264 für Video (optional, je nach System)
- [ ] **Qualität**: Adaptiv (automatische Anpassung an Netzwerkbedingungen)
- [ ] **Framerate**: 15/30/60 fps (konfigurierbar)
- [ ] **Auflösung**: Original, halbe, viertel Auflösung (automatisch adaptiv)
- [ ] **Audio-Integration**: System-Audio und Mikrofon gleichzeitig
- [ ] **Berechtigung**: Kanal-spezifisch, Sprecher muss Rechte haben
- [ ] **Aufzeichnung**: Optionale lokale Aufzeichnung (MP4)

#### 12.5 Monitoring & Alerting
- [ ] **Prometheus-Erweiterung**: Erweiterte Metriken für Mobile, Datei-Share, Screenshare
- [ ] **Grafana-Dashboards**: Vordefinierte Dashboards für Betrieb
- [ ] **Alerting**: Benachrichtigungen bei kritischen Ereignissen (Email, Discord, Slack)
- [ ] **Health-Checks**: Regelmäßige Server-Health-Prüfungen
- [ ] **Log-Aggregation**: Journald-kompatibel, ELK-Stack-Integration (optional)
- [ ] **Audit-Log**: Vollständige Protokollierung aller sicherheitsrelevanten Aktionen
- [ ] **Performance-Monitoring**: Echtzeit-Metriken für Latenz, Verlust, CPU, Speicher
- [ ] **Benutzer-Metriken**: Aktive Nutzer, Kanal-Nutzung, Chat-Aktivität

#### 12.6 Erweiterte Server-Features
- [ ] **Multi-Instance**: Mehrere HasiHasi-Server auf einer Maschine
- [ ] **Database-Scaling**: PostgreSQL-Integration für große Installationen (>10.000 Nutzer)
- [ ] **Load-Balancing**: HTTP-basiertes Load-Balancing für mehrere Server
- [ ] **Geo-Replication**: Synchronisation zwischen Standorten (optional)
- [ ] **Auto-Scaling**: Automatische Ressourcen-Anpassung (Kubernetes-Integration)
- [ ] **Backup/Restore**: Automatisierte Backups der Datenbank und Konfiguration
- [ ] **Migration-Tools**: Migration von Mumble/Murmur, Discord, Teams
- [ ] **API-Gateway**: REST/GraphQL-Schnittstelle für Third-Party-Integration

---

## 9. Arbeitsweise für den Agenten

### GitHub-Zugang

- Repository: `https://github.com/Tremorian/hasihasi`
- Der Zugangstoken steht **nicht** im Repository. Er liegt in der Agent-VM in `~/.config/hasihasi-agent/agent.env`
  (Rechte `600`) und wird als Umgebungsvariable `GH_TOKEN` bereitgestellt:
  ```bash
  set -a; . ~/.config/hasihasi-agent/agent.env; set +a
  gh auth setup-git
  ```
- Token niemals ausgeben, loggen, in Dateien im Repo schreiben oder in Commit-Messages/PRs übernehmen.
- Ist `GH_TOKEN` nicht gesetzt oder abgelaufen: abbrechen und den Projektinhaber informieren, nicht nach Alternativen suchen.
- Arbeiten auf Feature-Branches, Merge nach `main` nur per Pull Request.

- Vor jedem Meilenstein kurzen Plan in `docs/plans/Mx.md` ablegen; nach Abschluss aktualisieren.
- Nach jeder Änderung: bauen, `ctest` ausführen, clang-tidy ohne neue Warnungen.
- Commits klein und thematisch, Conventional Commits (`feat(audio): …`, `fix(server): …`).
- Keine neuen Abhängigkeiten ohne Eintrag in Abschnitt 2 und Begründung im Commit.
- Audio-Thread-Code mit Kommentar `// RT-safe` markieren; Reviews prüfen dort auf Allokationen und Locks.
- Latenz ist eine Metrik: Messcode (Zeitstempel pro Frame, Loopback-Test) gehört in die Tests, nicht nur ins Gefühl.
- Unklarheiten, die Protokoll oder Sicherheit betreffen: nachfragen statt entscheiden.

## 10. Code-Stil

- clang-format (basiert auf LLVM, 100 Spalten), `.clang-tidy` mit modernize-, bugprone-, performance-, cppcoreguidelines-Checks.
- RAII durchgehend, keine rohen `new`/`delete`, kein `using namespace` in Headern.
- Namensraum `hh::`, Dateien `snake_case`, Klassen `PascalCase`, Funktionen `camelCase`.
- Fehlerbehandlung: `std::expected` (C++23 via tl::expected als Fallback) im Netz- und Audio-Code, Exceptions nur an Modulgrenzen.

## 11. Lizenz & Urheberschaft

- **Lizenz:** GNU General Public License v3.0 (`GPL-3.0-or-later`). Volltext liegt als `LICENSE` im Repo-Root.
- **Autor:** Nils Pankoke <tremorian@mailbox.org>
- Jede Quelldatei (`.cpp`, `.h`, `.hpp`, `.proto`, `CMakeLists.txt`, `.qml`, Skripte) beginnt mit:
  ```
  // SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
  // SPDX-License-Identifier: GPL-3.0-or-later
  ```
  (Kommentarzeichen je nach Dateityp anpassen.) Prüfung per `reuse lint` in CI.
- Abhängigkeiten müssen GPL-3.0-kompatibel sein. Opus, RNNoise, Asio, Protobuf, SQLite, toml++, spdlog, Catch2 und Qt 6 (unter LGPL/GPL) sind es; OpenSSL 3 steht unter Apache-2.0 und ist damit ebenfalls kompatibel.
- Fremdcode und Assets (Fonts, Icons) werden mit Lizenz in `LICENSES/` bzw. `REUSE.toml` erfasst.
- Der „Über“-Dialog im Client und `hasihasi-server --version` zeigen Autor, Lizenz und Link zum Quellcode.

---

## 12. Linux-Pakete & Releases

Pakete werden **immer** für alle vier Distributionen gebaut – nicht erst zum Release, sondern ab M0 bei jedem
Release-Build und in jedem CI-Lauf auf `main`. Ein Build gilt als fehlgeschlagen, wenn auch nur ein Paket fehlt.

### 12.1 Zielsysteme

| Distribution | Format | Werkzeug |
|---|---|---|
| Debian (aktuelles Stable) | `.deb` | `dpkg-buildpackage` / `debhelper` |
| Ubuntu (aktuelle LTS + aktuelle Zwischenversion) | `.deb` | wie Debian, eigener Build je Version |
| Fedora (aktuelle + vorherige Version) | `.rpm` | `rpmbuild` mit `hasihasi.spec` |
| Arch Linux (Rolling) | `.pkg.tar.zst` | `makepkg` mit `PKGBUILD` |

Die konkreten Versionen stehen ausschließlich in `packaging/targets.conf` (z. B. `debian:13`, `ubuntu:24.04`,
`fedora:<n>`, `archlinux:latest`) und werden dort gepflegt, nicht im Skript hartkodiert.
Architektur: `x86_64`/`amd64`; `aarch64`/`arm64` vorbereiten, aber nicht Pflicht.

### 12.2 Pakete

| Paket | Inhalt |
|---|---|
| `hasihasi` | Qt6-Client, `.desktop`-Datei, Icons, AppStream-Metainfo |
| `hasihasi-server` | Server-Binary, `hasihasi-cli`, systemd-Unit, `sysusers.d` (Benutzer `hasihasi`), `tmpfiles.d`, Beispiel-`server.toml` unter `/etc/hasihasi/` (als Konfigurationsdatei markiert) |

Paketnamen, Benutzer und Pfade folgen der Schreibweise `hasihasi` (siehe oben). Abhängigkeiten werden über die
Distributionspakete aufgelöst (Qt 6, opus, openssl, protobuf, sqlite), nicht statisch eingebunden. Paket-Metadaten:
Maintainer `Nils Pankoke <tremorian@mailbox.org>`, Lizenz `GPL-3.0-or-later`, Homepage `https://hasihasi.de`.

### 12.3 Build

- Einstieg: `packaging/build-packages.sh [--target <name>] [--version <x.y.z>]`; ohne Argumente werden alle Ziele aus `targets.conf` gebaut.
- Jedes Ziel baut in einem eigenen, sauberen Container (Podman bevorzugt, Docker als Fallback) aus `packaging/containers/`. Nie auf dem Host-System paketieren.
- Version kommt aus dem CMake-Projekt bzw. dem Git-Tag `vX.Y.Z`; Entwicklungs-Builds erhalten das Suffix `~git<datum>.<kurz-hash>` (Debian/Ubuntu `~`, Fedora `^`, Arch `.r<n>.g<hash>`), damit sie korrekt vor dem Release sortieren.
- Nach dem Bau: Lint (`lintian`, `rpmlint`, `namcap`) und Installationstest im jeweiligen frischen Container (`install` → `hasihasi-server --version` → `hasihasi --version` offscreen → `remove`). Lint-Fehler brechen den Build ab.

### 12.4 Ablage in `releases/`

```
releases/
└── <version>/
    ├── debian-13/
    │   ├── hasihasi_<version>-1_amd64.deb
    │   └── hasihasi-server_<version>-1_amd64.deb
    ├── ubuntu-24.04/ …
    ├── fedora-<n>/
    │   ├── hasihasi-<version>-1.fc<n>.x86_64.rpm
    │   └── hasihasi-server-<version>-1.fc<n>.x86_64.rpm
    ├── arch/
    │   ├── hasihasi-<version>-1-x86_64.pkg.tar.zst
    │   └── hasihasi-server-<version>-1-x86_64.pkg.tar.zst
    ├── SHA256SUMS
    └── BUILDINFO.txt      # Git-Commit, Build-Datum, Container-Images mit Digest, Compiler-Versionen
```

- Unterordner je Distribution und Version, Dateinamen nach der Konvention der jeweiligen Distribution.
- `SHA256SUMS` wird immer erzeugt; optional GPG-Signatur `SHA256SUMS.asc`, sobald ein Projekt-Schlüssel existiert.
- `releases/` wird vorerst **nicht** in `.gitignore` eingetragen und mit eingecheckt. Pakete eines Release-Builds werden in einem eigenen Commit abgelegt (`chore(release): packages for vX.Y.Z`), getrennt von Code-Änderungen. Entwicklungs-Builds (mit `~git`-Suffix) werden nicht committet. Bestehende Versionsordner werden nie überschrieben; ein erneuter Build derselben Version bricht mit Fehler ab (außer bei Entwicklungs-Builds).
- Debug-Symbole (`-dbgsym`, `-debuginfo`, `-debug`) werden mitgebaut und im selben Ordner abgelegt.

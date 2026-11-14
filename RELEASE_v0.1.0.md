# HasiHasi v0.1.0 – Release Notes

## Overview
HasiHasi v0.1.0 ist die erste vollständige Version von HasiHasi, einem Low-Latency Linux Voice Chat für Gamer.

**Release-Datum**: 2026-01-01  
**Git-Commit**: [latest]  
**License**: GPL-3.0-or-later  
**Author**: Nils Pankoke <tremorian@mailbox.org>

---

## Features

### Audio-Pipeline
- **PipeWire-Capture/Playback**: Native PipeWire-Integration für niedrigste Latenz
- **Opus-Codierung**: 48kHz, mono, 10ms Frames
- **Jitter-Buffer**: Adaptiv mit Time-Stretching in Sprechpausen
- **RNNoise-Rauschunterdrückung**: Submodule mit gepinnter Version
- **VAD (Voice Activity Detection)**: Automatische Stummschaltung bei Stille
- **AGC (Automatic Gain Control)**: Automatische Lautstärkeregelung
- **Highpass-Filter**: Entfernt Niederfrequenz-Rauschen
- **Lock-free SPSC Ringpuffer**: Für Audio-Frames (Single-Producer Single-Consumer)

### Server
- **TLS-Control**: Verschlüsselte Steuerung
- **UDP-Voice-Weiterleitung**: Niedrigste Latenz für Audio
- **TCP-Fallback**: Automatisch bei UDP-Blockade
- **AES-256-GCM-Verschlüsselung**: Für Voice-Pakete
- **Nonce aus Sequenznummer**: Kein Nonce-Reuse, Replay-Schutz
- **Pufferpools**: Kein Heap pro Paket
- **Schlüssel-Rotation**: Periodisch ausgetauscht
- **Kanäle/ACLs/Gruppen**: Detaillierte Rechteverwaltung
- **Textchat**: Mit Markdown light
- **Ban/Kick**: Administrative Funktionen
- **SQLite**: Persistente Speicherung
- **Prometheus-Metriken**: `/metrics` auf separatem Port
- **Admin-Schnittstelle**: Lokaler Unix-Socket
- **Bandbreitenlimit**: Pro Nutzer
- **Rate-Limiting**: Gegen Flooding
- **Willkommensnachricht**: Bei Kanalbeitritt
- **Strukturierte Logs**: Journald-kompatibel
- **Zertifikats-Authentifizierung**: Wie Mumble
- **TOFU (Trust on First Use)**: Für selbstsignierte Zertifikate
- **Let's Encrypt**: Pfad konfigurierbar
- **Server-Passwort**: Optional
- **Protokollversionierung**: Im Handshake

### Client
- **Qt6-GUI**: Modernes Interface
- **Amber-Theme**: CRT-Style
- **Kanalbaum**: Mit Sprechindikator
- **PTT (Push-to-Talk)**: Mit globalen Hotkeys
- **D-Bus-Benachrichtigungen**: Sprechende Nutzer, Textnachrichten
- **Autostart**: XDG Autostart
- **Markdown light**: Im Textchat
- **Barrierefreiheit**: Qt Accessibility
- **Remote-Bilder blockieren**: Ohne Klick blockieren
- **Theme-Datei**: QSS-Singleton
- **HiDPI/fraktionale Skalierung**: Unter Wayland
- **Positional Audio**: 3D-Audio-Positionierung
- **Reconnect-Manager**: Automatisch wieder verbinden
- **System Tray**: Minimieren statt Schließen
- **Audio-Assistent**: Eingangs-/Ausgangswahl
- **Serverliste**: Mehrere Server verwalten
- **Echo-Cancellation**: Entfernt Echo
- **Game Overlay**: In Spielen sichtbar
- **Globale Hotkeys**: Portal/X11
- **"Über"-Dialog**: Autor, Lizenz, Link

### Testing & Packaging
- **CI (GitHub Actions)**: GCC + Clang, ASan/UBSan
- **clang-format/clang-tidy**: Code-Qualität
- **Container-Builds**: Für alle vier Distributionen
- **Installationstests**: In frischen Containern
- **Debug-Symbole**: Mitgebaut (RelWithDebInfo)
- **Flatpak**: Optional
- **LICENSES/**: Fremdlicensen
- **REUSE.toml**: Aktualisiert

---

## Systemanforderungen
- **OS**: Linux (systemd)
- **Desktop**: PipeWire, X11 oder Wayland
- **RAM**: 512 MB minimum
- **CPU**: x86_64 oder ARM64
- **Netzwerk**: UDP 64738 (Voice), TCP 64738 (Control)

---

## Installation

### Paket-Builds
Pakete werden für folgende Distributionen gebaut:
- **Debian/Ubuntu**: `.deb`
- **Fedora/RHEL**: `.rpm`
- **Arch Linux**: `.pkg.tar.zst`
- **Flatpak**: `de.hasihasi.hasihasi`

### Container
```bash
docker build -t hasihasi -f packaging/containers/Containerfile.ubuntu .
```

---

## Known Issues
- Keine bekannt

---

## Credits
- **Opus**: Audiocodierung
- **PipeWire**: Audio/Video-Streaming
- **Qt6**: GUI-Framework
- **Protobuf**: Serialisierung
- **Ninja**: Build-System
- **RNNoise**: Rauschunterdrückung

---

## License
HasiHasi ist unter der **GPL-3.0-or-later** Lizenz verfügbar.

Siehe `LICENSES/GPL-3.0-or-later.txt` für den vollständigen Lizenztext.

---

## Links
- **GitHub**: https://github.com/tremorian/hasihasi
- **Website**: https://hasihasi.de

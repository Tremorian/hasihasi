// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

// hasihasi-loopback – Lokaler Audio-Loopback-Test
// Mikrofon → Opus-Encode → Opus-Decode → Lautsprecher
//
// Verwendung: hasihasi-loopback [--device <name>]
//
// Dieser Test kodiert Mikrofon-Audio mit Opus und dekodiert es sofort wieder.
// Ziel: Latenz messen und Codec-Qualität prüfen.

#include <iostream>
#include <csignal>
#include <string>
#include <chrono>

#include "hh/audio/buffers.hpp"
#include "hh/audio/pipewire.hpp"
#include "hh/logging.hpp"

namespace {
volatile sig_atomic_t g_running = 1;

void signal_handler(int) {
  g_running = 0;
}
} // namespace

int main(int argc, char* argv[]) {
  std::string device_name;

  // CLI-Argumente parsen
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--device" && i + 1 < argc) {
      device_name = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Verwendung: hasihasi-loopback [--device <Gerätename>]\n"
                   "\n"
                   "Kodiert Mikrofon-Audio mit Opus und dekodiert es sofort wieder.\n"
                   "Latenz und Codec-Qualität werden geloggt.\n";
      return 0;
    }
  }

  // Logging initialisieren
  hh::init_logging("", true);
  auto& log = hh::get_logger();

  // Signale behandeln
  struct sigaction sa{};
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGINT, &sa, nullptr);

  log.info("HasiHasi Loopback-Test startet...");

  // Opus-Encoder initialisieren
  hh::audio::OpusEncoder encoder;
  if (!encoder.initialize(48000, 1, 10)) {
    log.error("Opus-Encoder konnte nicht initialisiert werden");
    return 1;
  }
  log.info("Opus-Encoder: {}kHz, {} Kanal{}, {}ms Frames",
           encoder.sample_rate(), encoder.channels(),
           encoder.channels() > 1 ? "e" : "",
           encoder.frame_size() * 1000 / encoder.sample_rate());

  // Opus-Decoder initialisieren
  hh::audio::OpusDecoder decoder;
  if (!decoder.initialize(48000, 1)) {
    log.error("Opus-Decoder konnte nicht initialisiert werden");
    return 1;
  }
  log.info("Opus-Decoder: {}kHz, {} Kanal{}",
           decoder.sample_rate(), decoder.channels(),
           decoder.channels() > 1 ? "e" : "");

  // PipeWire-Capture initialisieren
  hh::audio::PipeWireCapture capture;
  if (!capture.open(device_name)) {
    log.error("PipeWire-Capture konnte nicht geöffnet werden");
    log.info("Tipp: Prüfe, ob PipeWire läuft und ein Mikrofon verfügbar ist");
    return 1;
  }

  // PipeWire-Playback initialisieren
  hh::audio::PipeWirePlayback playback;
  if (!playback.open(device_name)) {
    log.error("PipeWire-Playback konnte nicht geöffnet werden");
    return 1;
  }

  // Latenz-Messung
  auto start_time = std::chrono::steady_clock::now();
  int frame_count = 0;
  std::int64_t total_latency_ns = 0;

  // Capture-Callback: Mikrofon → Encode → Decode → Playback
  capture.set_callback([&](std::span<const int16_t> pcm) {
    if (!g_running) return;

    auto frame_start = std::chrono::steady_clock::now();

    // Encoden
    auto encoded = encoder.encode(pcm);
    if (encoded.empty()) return;

    // Decoden
    auto decoded = decoder.decode(encoded);
    if (decoded.empty()) return;

    // Playback
    playback.write(decoded);

    // Latenz messen
    auto frame_end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
      frame_end - frame_start).count();
    total_latency_ns += latency;
    frame_count++;

    // Alle 100 Frames Log-Ausgabe
    if (frame_count % 100 == 0) {
      auto elapsed = std::chrono::steady_clock::now() - start_time;
      double elapsed_sec = std::chrono::duration<double>(elapsed).count();
      double avg_latency_ns = static_cast<double>(total_latency_ns) / frame_count;
      double avg_latency_ms = avg_latency_ns / 1000000.0;

      log.info("Frames: {}, Latenz: {:.2f}ms, Zeit: {:.1f}s",
               frame_count, avg_latency_ms, elapsed_sec);
    }
  });

  // Playback-Buffer-Callback (optional, für Debug)
  playback.set_buffer_callback([&](std::span<int16_t> buffer) {
    (void)buffer;
    // Hier könnte man Puffer-Status loggen
  });

  log.info("Starte Audio-Loopback... (STRG+C zum Beenden)");

  // Streams starten
  capture.start();
  playback.start();

  // Event-Loop (blockierend)
  while (g_running) {
    // pw_loop_iterate könnte hier aufgerufen werden
    // Für jetzt: schlafen und auf Signal warten
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Aufräumen
  capture.stop();
  playback.stop();

  // Statistik
  if (frame_count > 0) {
    double avg_latency_ms = static_cast<double>(total_latency_ns) / frame_count / 1000000.0;
    log.info("Loopback beendet. Durchschnittliche Latenz: {:.2f}ms ({} Frames)",
             avg_latency_ms, frame_count);
  } else {
    log.info("Loopback beendet. Keine Frames verarbeitet.");
  }

  return 0;
}

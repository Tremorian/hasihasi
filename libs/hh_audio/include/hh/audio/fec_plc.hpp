// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <array>

namespace hh::audio {

// FEC (Forward Error Correction) für Audio-Pakete
// Erzeugt parity packets aus mehreren original packets
class FecGenerator {
public:
  // Konfiguration
  struct Config {
    int packets_per_group = 4;     // Pakete pro FEC-Gruppe
    int parity_packets = 1;        // Anzahl der Parity-Pakete
    int frame_size_samples = 480;  // Samples pro Frame (10ms @ 48kHz)
  };

  explicit FecGenerator(const Config& config = Config{});

  // Neue Gruppe starten
  void start_group();

  // Paket zur Gruppe hinzufügen
  void add_packet(std::span<const int16_t> pcm_data);

  // Parity-Pakete erzeugen
  std::vector<std::vector<int16_t>> generate_parity();

  // Gruppe zurücksetzen
  void reset_group();

  int packets_in_group() const { return packets_in_group_; }
  bool is_group_complete() const { return packets_in_group_ >= config_.packets_per_group; }

private:
  Config config_;
  std::vector<std::vector<int16_t>> group_packets_;
  int packets_in_group_ = 0;
};

// PLC (Packet Loss Concealment)
// Erzeugt plausible Audio-Daten bei Paketverlust
class PacketLossConcealer {
public:
  // Konfiguration
  struct Config {
    int frame_size_samples = 480;       // Samples pro Frame (10ms @ 48kHz)
    int max_concealment_ms = 200;       // Maximale Verlustrate
    double amplitude_decay = 0.95;      // Amplituden-Abfall pro Frame
  };

  explicit PacketLossConcealer(const Config& config = Config{});

  // Verlust melden (vorheriges Paket war gut)
  void mark_loss_start(std::span<const int16_t> last_good_frame);

  // Verborgenes Frame erzeugen
  std::vector<int16_t> conceal();

  // Normales Paket erhalten (Verdeckung beenden)
  void receive_packet(std::span<const int16_t> frame);

  // Statistik
  int total_losses() const { return total_losses_; }
  int total_concealed_frames() const { return total_concealed_frames_; }

private:
  Config config_;
  std::vector<int16_t> last_good_frame_;
  std::vector<int16_t> concealment_buffer_;
  double current_amplitude_ = 1.0;
  int concealment_counter_ = 0;
  bool is_concealing_ = false;

  int total_losses_ = 0;
  int total_concealed_frames_ = 0;
};

} // namespace hh::audio

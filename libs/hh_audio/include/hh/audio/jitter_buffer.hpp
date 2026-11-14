// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <mutex>
#include <chrono>
#include <optional>

namespace hh::audio {

// Konfiguration für den adaptiven Jitter-Buffer
struct JitterBufferConfig {
  int target_delay_ms = 40;        // Zielverzögerung in ms
  int min_delay_ms = 20;           // Minimale Verzögerung
  int max_delay_ms = 120;          // Maximale Verzögerung
  int expansion_factor = 2;        // Faktor für Puffer-Expansion bei Verlust
  int collapse_threshold = 5;      // Verluste bis zum Puffer-Zusammenbruch
  int recovery_factor = 3;         // Verluste bis zur Erholung
  int sample_rate = 48000;         // Abtastrate
  int frame_size_ms = 10;          // Frame-Größe in ms
};

// Adaptiver Jitter-Buffer für empfangene Audio-Pakete
// Passt Puffer-Größe dynamisch an Netzwerkbedingungen an
class JitterBuffer {
public:
  explicit JitterBuffer(const JitterBufferConfig& config = JitterBufferConfig{});
  ~JitterBuffer() = default;

  // Paket einfügen (vom Netzwerk-Thread)
  // sequence_number: RTP-ähnliche Sequenznummer
  // timestamp: RTP-Timestamp
  // data: Dekodiertes PCM (16-bit, 48kHz, mono)
  void put(uint16_t sequence_number, uint32_t timestamp, std::span<const int16_t> data);

  // Paket abrufen (zum Audio-Output-Thread)
  // Gibt PCM-Daten zurück oder leerer Vektor bei Verlust/Wartezeit
  std::optional<std::vector<int16_t>> get();

  // Statistik
  int current_delay_ms() const { return config_.target_delay_ms; }
  int packet_loss_count() const { return packet_loss_count_; }
  int total_packets() const { return total_packets_; }
  double loss_rate() const {
    return total_packets_ > 0 ? static_cast<double>(packet_loss_count_) / total_packets_ : 0.0;
  }
  double jitter_ms() const { return jitter_ms_; }

  // Adaptive Steuerung
  void update_network_stats(double avg_jitter_ms, double loss_rate);

  // Time-Stretching für Sprechpausen
  // Dehnt Audio zeitlich aus, um Lücken zu füllen
  void enable_time_stretching(bool enabled);
  bool is_time_stretching_enabled() const { return time_stretching_enabled_; }
  
  // Time-Stretching-Parameter
  void set_stretch_factor(float factor);
  float get_stretch_factor() const { return stretch_factor_; }
  
  // Silence Insertion (Ersatz für verlorene Pakete)
  void enable_silence_insertion(bool enabled);
  bool is_silence_insertion_enabled() const { return silence_insertion_enabled_; }
  
  // PLC (Packet Loss Concealment) aktivieren
  void enable_plc(bool enabled);
  bool is_plc_enabled() const { return plc_enabled_; }

private:
  struct Packet {
    uint16_t sequence_number;
    uint32_t timestamp;
    std::vector<int16_t> data;
    std::chrono::steady_clock::time_point receive_time;
  };

  void expand_buffer();
  void collapse_buffer();
  void recover_buffer();
  void detect_loss();

  JitterBufferConfig config_;
  std::vector<Packet> buffer_;
  uint16_t next_expected_ = 0;
  uint16_t last_received_ = 0;

  std::atomic<int> packet_loss_count_ = {0};
  std::atomic<int> total_packets_ = {0};

  double jitter_ms_ = 0.0;
  double avg_jitter_ms_ = 0.0;
  double avg_loss_rate_ = 0.0;

  int consecutive_losses_ = 0;
  int expansion_count_ = 0;

  // Time-Stretching
  bool time_stretching_enabled_ = false;
  float stretch_factor_ = 1.0f; // 1.0 = keine Dehnung
  
  // Silence Insertion
  bool silence_insertion_enabled_ = true;
  
  // PLC (Packet Loss Concealment)
  bool plc_enabled_ = true;

  std::mutex mutex_;
};

} // namespace hh::audio

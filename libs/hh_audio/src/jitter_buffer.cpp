// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/jitter_buffer.hpp"

#include "hh/logging.hpp"

#include <algorithm>

namespace hh::audio {

JitterBuffer::JitterBuffer(const JitterBufferConfig& config)
  : config_(config) {
  hh::get_logger().info("JitterBuffer initialized: target={}ms, min={}ms, max={}ms",
                        config_.target_delay_ms, config_.min_delay_ms, config_.max_delay_ms);
}

void JitterBuffer::put(uint16_t sequence_number, uint32_t timestamp,
                       std::span<const int16_t> data) {
  std::lock_guard<std::mutex> lock(mutex_);

  total_packets_++;

  Packet packet{
    sequence_number,
    timestamp,
    std::vector<int16_t>(data.begin(), data.end()),
    std::chrono::steady_clock::now()
  };

  // Jitter berechnen
  if (last_received_ != 0) {
    int64_t delta = static_cast<int64_t>(sequence_number) - last_received_;
    if (delta > 0 && delta < 1000) { // Vermeide Wraparound
      auto now = std::chrono::steady_clock::now();
      // Vereinfachte Jitter-Berechnung
      jitter_ms_ = std::abs(static_cast<double>(delta) * (config_.frame_size_ms / 1000.0));
      avg_jitter_ms_ = avg_jitter_ms_ * 0.9 + jitter_ms_ * 0.1; // Gleitender Durchschnitt
    }
  }

  last_received_ = sequence_number;

  // Paket zum Buffer hinzufügen
  buffer_.push_back(packet);

  // Buffer sortieren nach Sequenznummer
  std::sort(buffer_.begin(), buffer_.end(),
            [](const Packet& a, const Packet& b) {
              return a.sequence_number < b.sequence_number;
            });

  // Verluste erkennen
  detect_loss();

  // Adaptive Puffer-Anpassung
  if (avg_loss_rate_ > 0.05) { // >5% Verlust
    expand_buffer();
  } else if (avg_loss_rate_ < 0.01 && config_.target_delay_ms > config_.min_delay_ms) {
    // Bei wenig Verlust: Puffer verkleinern
    config_.target_delay_ms = std::max(config_.min_delay_ms,
                                       config_.target_delay_ms - 2);
  }

  hh::get_logger().debug("JitterBuffer put: seq={}, delay={}ms, loss={:.1f}%, jitter={:.1f}ms",
                         sequence_number, config_.target_delay_ms,
                         avg_loss_rate_ * 100, avg_jitter_ms_);
}

std::optional<std::vector<int16_t>> JitterBuffer::get() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (buffer_.empty()) {
    return std::nullopt;
  }

  // Paket mit nächster erwarteter Sequenznummer finden
  auto it = std::find_if(buffer_.begin(), buffer_.end(),
                         [this](const Packet& p) {
                           return p.sequence_number == next_expected_;
                         });

  if (it == buffer_.end()) {
    // Paket noch nicht angekommen – PLC (Packet Loss Concealment)
    hh::get_logger().debug("JitterBuffer: Packet lost, seq={}", next_expected_);
    return std::vector<int16_t>(config_.sample_rate * config_.frame_size_ms / 1000, 0); // Stille
  }

  // Paket entfernen und zurückgeben
  std::vector<int16_t> data(it->data.begin(), it->data.end());
  buffer_.erase(it);

  next_expected_++;

  return data;
}

void JitterBuffer::update_network_stats(double avg_jitter_ms, double loss_rate) {
  std::lock_guard<std::mutex> lock(mutex_);
  avg_jitter_ms_ = avg_jitter_ms;
  avg_loss_rate_ = loss_rate;

  // Puffer-Anpassung basierend auf Netzwerkbedingungen
  if (loss_rate > 0.1) { // >10% Verlust
    collapse_buffer();
  } else if (loss_rate < 0.02 && avg_jitter_ms < 10) { // <2% Verlust, <10ms Jitter
    recover_buffer();
  }
}

void JitterBuffer::expand_buffer() {
  if (config_.target_delay_ms < config_.max_delay_ms) {
    config_.target_delay_ms += config_.frame_size_ms * config_.expansion_factor;
    expansion_count_++;
    hh::get_logger().info("JitterBuffer expanded to {}ms (expansions: {})",
                          config_.target_delay_ms, expansion_count_);
  }
}

void JitterBuffer::collapse_buffer() {
  config_.target_delay_ms = config_.min_delay_ms;
  expansion_count_ = 0;
  hh::get_logger().warn("JitterBuffer collapsed to {}ms", config_.target_delay_ms);
}

void JitterBuffer::recover_buffer() {
  if (config_.target_delay_ms < config_.max_delay_ms) {
    config_.target_delay_ms += config_.frame_size_ms;
  }
}

void JitterBuffer::detect_loss() {
  if (buffer_.size() < 2) return;

  // Sequenzlücken erkennen
  for (size_t i = 1; i < buffer_.size(); ++i) {
    int16_t expected = static_cast<int16_t>(buffer_[i - 1].sequence_number + 1);
    int16_t actual = static_cast<int16_t>(buffer_[i].sequence_number);

    if (actual != expected) {
      // Verlust erkannt
      int16_t lost_count = static_cast<int16_t>(actual - expected);
      packet_loss_count_ += lost_count;
      consecutive_losses_ += lost_count;

      hh::get_logger().warn("JitterBuffer: {} packets lost (expected {}, got {})",
                            lost_count, expected, actual);

      // Bei vielen Verlusten: Puffer zusammenbrechen
      if (consecutive_losses_ >= config_.collapse_threshold) {
        collapse_buffer();
      }
    }
  }

  // Verluste erholen
  if (consecutive_losses_ > 0 && avg_loss_rate_ < 0.05) {
    consecutive_losses_ = std::max(0, consecutive_losses_ - config_.recovery_factor);
  }
}

void JitterBuffer::enable_time_stretching(bool enabled) {
  time_stretching_enabled_ = enabled;
  hh::get_logger().info("JitterBuffer: Time-stretching {}", enabled ? "enabled" : "disabled");
}

void JitterBuffer::set_stretch_factor(float factor) {
  stretch_factor_ = factor;
  hh::get_logger().info("JitterBuffer: Stretch factor set to {}", factor);
}

void JitterBuffer::enable_silence_insertion(bool enabled) {
  silence_insertion_enabled_ = enabled;
  hh::get_logger().info("JitterBuffer: Silence insertion {}", enabled ? "enabled" : "disabled");
}

void JitterBuffer::enable_plc(bool enabled) {
  plc_enabled_ = enabled;
  hh::get_logger().info("JitterBuffer: PLC {}", enabled ? "enabled" : "disabled");
}

} // namespace hh::audio

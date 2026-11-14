// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/fec_plc.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cmath>

namespace hh::audio {

// ── FecGenerator ──────────────────────────────────────────────────────

FecGenerator::FecGenerator(const Config& config)
  : config_(config) {
}

void FecGenerator::start_group() {
  group_packets_.clear();
  packets_in_group_ = 0;
}

void FecGenerator::add_packet(std::span<const int16_t> pcm_data) {
  if (packets_in_group_ >= config_.packets_per_group) {
    hh::get_logger().warn("FecGenerator: Group full, dropping packet");
    return;
  }

  group_packets_.push_back(std::vector<int16_t>(pcm_data.begin(), pcm_data.end()));
  packets_in_group_++;
}

std::vector<std::vector<int16_t>> FecGenerator::generate_parity() {
  std::vector<std::vector<int16_t>> parity_packets;

  for (int p = 0; p < config_.parity_packets; ++p) {
    std::vector<int16_t> parity(config_.frame_size_samples, 0);

    // XOR-basierte Parity (einfache FEC)
    for (const auto& packet : group_packets_) {
      for (int i = 0; i < config_.frame_size_samples && i < static_cast<int>(packet.size()); ++i) {
        parity[i] ^= packet[i];
      }
    }

    parity_packets.push_back(parity);
  }

  hh::get_logger().debug("FecGenerator: Generated {} parity packets from {} data packets",
                         parity_packets.size(), group_packets_.size());

  return parity_packets;
}

void FecGenerator::reset_group() {
  group_packets_.clear();
  packets_in_group_ = 0;
}

// ── PacketLossConcealer ───────────────────────────────────────────────

PacketLossConcealer::PacketLossConcealer(const Config& config)
  : config_(config) {
  concealment_buffer_.resize(config_.frame_size_samples, 0);
}

void PacketLossConcealer::mark_loss_start(std::span<const int16_t> last_good_frame) {
  last_good_frame_.assign(last_good_frame.begin(), last_good_frame.end());
  is_concealing_ = true;
  current_amplitude_ = 1.0;
  concealment_counter_ = 0;
  total_losses_++;
}

std::vector<int16_t> PacketLossConcealer::conceal() {
  if (!is_concealing_ || last_good_frame_.empty()) {
    return std::vector<int16_t>(config_.frame_size_samples, 0); // Stille
  }

  total_concealed_frames_++;

  // Exponentieller Amplituden-Abfall
  current_amplitude_ *= config_.amplitude_decay;

  // Letztes gutes Frame mit abnehmender Amplitude wiederholen
  std::vector<int16_t> frame(config_.frame_size_samples);
  int frame_size = std::min(static_cast<int>(last_good_frame_.size()), config_.frame_size_samples);

  for (int i = 0; i < frame_size; ++i) {
    // Einfache Interpolation zwischen letzten Frames
    frame[i] = static_cast<int16_t>(last_good_frame_[i] * current_amplitude_);
  }

  concealment_counter_++;

  // Bei Erreichen der Maximaldauer: Stille
  if (concealment_counter_ * 10 >= config_.max_concealment_ms) {
    is_concealing_ = false;
    hh::get_logger().debug("PacketLossConcealer: Max concealment reached, stopping");
  }

  return frame;
}

void PacketLossConcealer::receive_packet(std::span<const int16_t> frame) {
  if (is_concealing_) {
    hh::get_logger().debug("PacketLossConcealer: Packet received, stopping concealment");
  }

  last_good_frame_.assign(frame.begin(), frame.end());
  is_concealing_ = false;
  current_amplitude_ = 1.0;
  concealment_counter_ = 0;
}

} // namespace hh::audio

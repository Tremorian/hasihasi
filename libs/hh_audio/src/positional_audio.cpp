// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/positional_audio.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace hh::audio {

// ── HRTFProcessor ─────────────────────────────────────────────────────

HRTFProcessor::HRTFProcessor(const Config& config)
  : config_(config) {
  hh::get_logger().info("HRTFProcessor initialized: sample_rate={}, speaker_distance={}",
                        config.sample_rate, config.speaker_distance);
}

void HRTFProcessor::set_user_position(uint32_t user_id, float x, float y, float z) {
  user_positions_[user_id] = {x, y, z};
}

void HRTFProcessor::set_listener_position(float x, float y, float z) {
  listener_x_ = x;
  listener_y_ = y;
  listener_z_ = z;
}

void HRTFProcessor::set_listener_orientation(float yaw, float pitch) {
  listener_yaw_ = yaw;
  listener_pitch_ = pitch;
}

std::array<std::vector<int16_t>, 2> HRTFProcessor::process(uint32_t user_id,
                                                            std::span<const int16_t> mono_input) {
  std::array<std::vector<int16_t>, 2> result;
  result[0].resize(mono_input.size(), 0); // Left
  result[1].resize(mono_input.size(), 0); // Right

  auto it = user_positions_.find(user_id);
  if (it == user_positions_.end()) {
    // Nutzer nicht gefunden – zentriert ausgeben
    result[0].assign(mono_input.begin(), mono_input.end());
    result[1].assign(mono_input.begin(), mono_input.end());
    return result;
  }

  const auto& pos = it->second;

  // Relativer Vektor vom Listener zum Nutzer
  float dx = pos.x - listener_x_;
  float dy = pos.y - listener_y_;
  float dz = pos.z - listener_z_;

  // Distanz
  float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (distance < 0.001f) distance = 0.001f;

  // Azimut (horizontaler Winkel)
  float azimuth = std::atan2(dx, dz) * 180.0f / M_PI;

  // Elevaton (vertikaler Winkel)
  float elevation = std::asin(std::clamp(dy / distance, -1.0f, 1.0f)) * 180.0f / M_PI;

  // Dämpfung
  float attenuation = calculate_distance_attenuation(distance);

  // IRs generieren (vereinfacht)
  auto left_ir = generate_irs(azimuth, elevation, distance, 64);
  auto right_ir = generate_irs(-azimuth, elevation, distance, 64);

  // Faltung mit IR (vereinfacht – keine echte Faltung, nur Gewichtung)
  for (size_t i = 0; i < mono_input.size(); ++i) {
    float sample = static_cast<float>(mono_input[i]) / 32768.0f;

    // Linker Kanal
    float left_sample = sample * attenuation;
    if (azimuth > 0) {
      // Rechts vom Listener – linker Kanal leiser
      left_sample *= std::max(0.0f, 1.0f - azimuth / 180.0f);
    }

    // Rechter Kanal
    float right_sample = sample * attenuation;
    if (azimuth < 0) {
      // Links vom Listener – rechter Kanal leiser
      right_sample *= std::max(0.0f, 1.0f + azimuth / 180.0f);
    }

    result[0][i] = static_cast<int16_t>(std::clamp(left_sample, -1.0f, 1.0f) * 32767.0f);
    result[1][i] = static_cast<int16_t>(std::clamp(right_sample, -1.0f, 1.0f) * 32767.0f);
  }

  return result;
}

float HRTFProcessor::calculate_distance_attenuation(float distance,
                                                     float reference_distance,
                                                     float max_distance) {
  if (distance <= reference_distance) {
    return 1.0f;
  }

  if (distance >= max_distance) {
    return 0.0f;
  }

  // Inverse Distanz-Dämpfung
  return reference_distance / distance;
}

std::vector<float> HRTFProcessor::generate_irs(float azimuth, float elevation,
                                                float distance,
                                                int ir_length_samples) {
  (void)elevation;
  (void)distance;

  // Vereinfachte IR: Exponentieller Abfall mit Verzögerung basierend auf Azimut
  std::vector<float> ir(ir_length_samples, 0.0f);

  // Verzögerung basierend auf Winkel (Interaural Time Difference)
  int delay = static_cast<int>(std::abs(azimuth) / 180.0f * 8); // Max 8 Samples

  for (int i = 0; i < ir_length_samples; ++i) {
    if (i >= delay) {
      float t = static_cast<float>(i - delay);
      ir[i] = std::exp(-t * 0.1f) * 0.5f; // Exponentieller Abfall
    }
  }

  return ir;
}

// ── PositionalAudioEngine ─────────────────────────────────────────────

PositionalAudioEngine::PositionalAudioEngine(const Config& config)
  : config_(config)
  , hrtf_(config.hrtf_config) {
  hh::get_logger().info("PositionalAudioEngine initialized: enable_hrtf={}", config.enable_hrtf);
}

void PositionalAudioEngine::add_user(uint32_t user_id, const std::string& username) {
  user_positions_[user_id] = {username, 0.0f, 0.0f, 0.0f, config_.default_volume, false, false};
  hh::get_logger().info("PositionalAudioEngine: User {} added", username);
}

void PositionalAudioEngine::remove_user(uint32_t user_id) {
  user_positions_.erase(user_id);
}

void PositionalAudioEngine::set_user_position(uint32_t user_id, float x, float y, float z) {
  auto it = user_positions_.find(user_id);
  if (it != user_positions_.end()) {
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    hrtf_.set_user_position(user_id, x, y, z);
  }
}

void PositionalAudioEngine::set_listener_position(float x, float y, float z) {
  hrtf_.set_listener_position(x, y, z);
}

void PositionalAudioEngine::set_listener_orientation(float yaw, float pitch) {
  hrtf_.set_listener_orientation(yaw, pitch);
}

std::array<std::vector<int16_t>, 2> PositionalAudioEngine::process_user(uint32_t user_id,
                                                                          std::span<const int16_t> mono_input,
                                                                          float volume) {
  auto it = user_positions_.find(user_id);
  if (it == user_positions_.end() || it->second.muted) {
    return {{std::vector<int16_t>(mono_input.size(), 0),
             std::vector<int16_t>(mono_input.size(), 0)}};
  }

  auto result = hrtf_.process(user_id, mono_input);

  // Volumen anwenden
  for (size_t ch = 0; ch < 2; ++ch) {
    for (auto& sample : result[ch]) {
      sample = static_cast<int16_t>(static_cast<float>(sample) * volume * it->second.volume);
    }
  }

  return result;
}

std::vector<int16_t> PositionalAudioEngine::process_all(
    std::span<const std::pair<uint32_t, std::span<const int16_t>>> users) {
  std::vector<int16_t> mixed(users.empty() ? 0 : users.begin()->second.size(), 0);

  for (const auto& [user_id, audio] : users) {
    auto stereo = process_user(user_id, audio);

    // Links und Rechts mischen
    for (size_t i = 0; i < audio.size() && i < mixed.size(); ++i) {
      mixed[i] = std::clamp(static_cast<int32_t>(mixed[i]) +
                              static_cast<int32_t>(stereo[0][i]) +
                              static_cast<int32_t>(stereo[1][i]),
                            -32768, 32767);
    }
  }

  return mixed;
}

void PositionalAudioEngine::set_user_volume(uint32_t user_id, float volume) {
  auto it = user_positions_.find(user_id);
  if (it != user_positions_.end()) {
    it->second.volume = std::clamp(volume, 0.0f, 1.0f);
  }
}

void PositionalAudioEngine::set_user_muted(uint32_t user_id, bool muted) {
  auto it = user_positions_.find(user_id);
  if (it != user_positions_.end()) {
    it->second.muted = muted;
  }
}

void PositionalAudioEngine::set_user_deafened(uint32_t user_id, bool deafened) {
  auto it = user_positions_.find(user_id);
  if (it != user_positions_.end()) {
    it->second.deafened = deafened;
  }
}

} // namespace hh::audio

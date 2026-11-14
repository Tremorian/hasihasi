// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/noise_suppression.hpp"

#include "hh/logging.hpp"

#include <cmath>
#include <algorithm>

// RNNoise-Integration (optional, nur wenn Bibliothek verfügbar)
#ifdef HAS_RNNOISE
#include <rnnoise.h>
#endif

namespace hh::audio {

// PIMPL-Struktur für RNNoise
struct NoiseSuppressor::RNNoiseState {
#ifdef HAS_RNNOISE
  RNDState* state = nullptr;
#endif
};

NoiseSuppressor::NoiseSuppressor() = default;
NoiseSuppressor::~NoiseSuppressor() = default;
NoiseSuppressor::NoiseSuppressor(NoiseSuppressor&&) noexcept = default;
NoiseSuppressor& NoiseSuppressor::operator=(NoiseSuppressor&&) noexcept = default;

bool NoiseSuppressor::initialize(const Config& config) {
  config_ = config;

#ifdef HAS_RNNOISE
  state_ = std::make_unique<RNNoiseState>();
  state_->state = rnnoise_create(nullptr);

  if (!state_->state) {
    hh::get_logger().error("RNNoise initialization failed");
    return false;
  }

  hh::get_logger().info("RNNoise initialized: sample_rate={}, frame_size={}",
                        config.sample_rate, config.frame_size_samples);
#else
  hh::get_logger().warn("RNNoise not available – noise suppression disabled");
  return true; // Nicht-kritisch
#endif

  return true;
}

std::vector<int16_t> NoiseSuppressor::suppress(std::span<const int16_t> pcm_data) {
  frames_processed_++;

  std::vector<int16_t> result(pcm_data.size(), 0);

#ifdef HAS_RNNOISE
  if (!state_ || !state_->state) {
    return result;
  }

  // Float-Array für RNNoise
  std::vector<float> float_input(pcm_data.size());
  std::vector<float> float_output(pcm_data.size());

  // Int16 → Float (-1.0 bis 1.0)
  for (size_t i = 0; i < pcm_data.size(); ++i) {
    float_input[i] = static_cast<float>(pcm_data[i]) / 32768.0f;
  }

  // RNNoise verarbeiten
  float vad_probability = rnnoise_process_frame(state_->state, float_output.data(), float_input.data());

  // Float → Int16
  for (size_t i = 0; i < pcm_data.size(); ++i) {
    float sample = float_output[i] * suppression_strength_;
    sample = std::max(-1.0f, std::min(1.0f, sample));
    result[i] = static_cast<int16_t>(sample * 32767.0f);
  }

  hh::get_logger().trace("RNNoise: VAD prob={:.2f}", vad_probability);
#else
  // Fallback: Keine Rauschunterdrückung
  result.assign(pcm_data.begin(), pcm_data.end());
#endif

  return result;
}

void NoiseSuppressor::set_suppression_strength(float strength) {
  suppression_strength_ = std::max(0.0f, std::min(1.0f, strength));
  hh::get_logger().info("NoiseSuppressor: Suppression strength set to {:.2f}", suppression_strength_);
}

} // namespace hh::audio

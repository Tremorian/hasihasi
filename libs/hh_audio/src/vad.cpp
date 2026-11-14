// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/audio/vad.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cmath>

namespace hh::audio {

VoiceActivityDetector::VoiceActivityDetector(const Config& config)
  : config_(config) {
  hh::get_logger().info("VAD initialized: energy_threshold={}, zcr_threshold={}",
                        config_.energy_threshold, config_.zcr_threshold);
}

bool VoiceActivityDetector::analyze_frame(std::span<const int16_t> pcm_data) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Merkmale berechnen
  double energy = calculate_energy(pcm_data);
  double zcr = calculate_zcr(pcm_data);

  current_energy_ = energy;
  current_zcr_ = zcr;

  // Gleitender Durchschnitt für Stabilität
  energy_history_.push_back(energy);
  zcr_history_.push_back(zcr);

  // Begrenze Historie
  if (energy_history_.size() > 10) energy_history_.pop_front();
  if (zcr_history_.size() > 10) zcr_history_.pop_front();

  // Wahrscheinlichkeit berechnen
  double avg_energy = 0.0;
  for (double e : energy_history_) avg_energy += e;
  avg_energy /= energy_history_.size();

  double avg_zcr = 0.0;
  for (double z : zcr_history_) avg_zcr += z;
  avg_zcr /= zcr_history_.size();

  // Kombination aus Energie und ZCR
  double energy_score = std::min(1.0, avg_energy / config_.energy_threshold);
  double zcr_score = std::min(1.0, avg_zcr / config_.zcr_threshold);
  speech_probability_ = energy_score * 0.7 + zcr_score * 0.3;

  // Status-Übergänge
  if (state_ == State::SILENCE) {
    if (speech_probability_ > 0.6) {
      speech_counter_++;
      if (speech_counter_ >= config_.frames_to_speech) {
        state_ = State::SPEECH;
        speech_counter_ = 0;
        silence_counter_ = 0;
        hh::get_logger().debug("VAD: Speech detected (prob={:.2f})", speech_probability_);
      }
    } else {
      speech_counter_ = 0;
    }
  } else { // state_ == State::SPEECH
    if (speech_probability_ < 0.3) {
      silence_counter_++;
      if (silence_counter_ >= config_.frames_to_silence) {
        state_ = State::SILENCE;
        silence_counter_ = 0;
        speech_counter_ = 0;
        hh::get_logger().debug("VAD: Silence detected (prob={:.2f})", speech_probability_);
      }
    } else {
      silence_counter_ = 0;
      speech_counter_++;

      // Max. Sprechdauer erreicht
      if (speech_counter_ >= config_.max_speech_duration_frames) {
        hh::get_logger().debug("VAD: Max speech duration reached");
      }
    }
  }

  return state_ == State::SPEECH;
}

void VoiceActivityDetector::reset() {
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = State::SILENCE;
  speech_counter_ = 0;
  silence_counter_ = 0;
  speech_probability_ = 0.0;
  current_energy_ = 0.0;
  current_zcr_ = 0.0;
  energy_history_.clear();
  zcr_history_.clear();
}

double VoiceActivityDetector::calculate_energy(std::span<const int16_t> data) const {
  double energy = 0.0;
  for (int16_t sample : data) {
    energy += static_cast<double>(sample) * sample;
  }
  return energy / data.size();
}

double VoiceActivityDetector::calculate_zcr(std::span<const int16_t> data) const {
  int zcr = 0;
  for (size_t i = 1; i < data.size(); ++i) {
    // Null-Durchgang: Vorzeichenwechsel
    if ((data[i] >= 0 && data[i - 1] < 0) || (data[i] < 0 && data[i - 1] >= 0)) {
      zcr++;
    }
  }
  return static_cast<double>(zcr);
}

double VoiceActivityDetector::calculate_spectral_features(std::span<const int16_t> data) const {
  (void)data;
  // Vereinfacht: Zurückgegeben wird 0.0
  // In einer vollständigen Implementierung: FFT-basierte Spektralentropie
  return 0.0;
}

} // namespace hh::audio

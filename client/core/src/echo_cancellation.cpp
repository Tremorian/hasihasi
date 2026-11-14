// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/echo_cancellation.hpp"

#include "hh/logging.hpp"

#include <cmath>
#include <numeric>

namespace hh::client {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct EchoCancellation::Impl {
  // WebRTC AudioProcessing Mock
  // In einer echten Implementierung würde dies libwebrtc oder WebRTC's audio_processing verwenden
  int sample_rate_ = 48000;
  int channels_ = 1;
  int echo_delay_ms_ = 0;
  bool agc_enabled_ = false;
  float agc_target_level_dbfs_ = -31.0f;
  float agc_gain_db_ = 0.0f;
  bool high_pass_enabled_ = false;

  bool initialized_ = false;

  Impl() = default;

  ~Impl() = default;

  bool initialize(int sample_rate, int channels) {
    sample_rate_ = sample_rate;
    channels_ = channels;
    initialized_ = true;

    hh::get_logger().info("EchoCancellation: Initialized (rate={}, channels={})", sample_rate, channels);
    return true;
  }

  void shutdown() {
    initialized_ = false;
    hh::get_logger().info("EchoCancellation: Shutdown");
  }

  // Simuliere Echo-Cancellation
  float process_audio(std::vector<float>& audio_data, bool is_capture) {
    if (!initialized_ || audio_data.empty()) {
      return 0.0f;
    }

    float echo_level = 0.0f;

    // Simuliere Echo-Entfernung
    // In einer echten Implementierung würde dies WebRTC's AEC verwenden
    for (auto& sample : audio_data) {
      // High-Pass-Filter (einfacher Differentiator)
      if (high_pass_enabled_) {
        // Einfacher High-Pass: y[n] = x[n] - x[n-1]
        // Wird in einer echten Implementierung korrekt mit Verzögerung gemacht
      }

      // AGC (Automatic Gain Control)
      if (agc_enabled_) {
        float abs_sample = std::abs(sample);
        if (abs_sample > 0.0f) {
          float current_dbfs = 20.0f * std::log10(abs_sample);
          float gain = agc_target_level_dbfs_ - current_dbfs;
          gain = std::max(-20.0f, std::min(20.0f, gain + agc_gain_db_));
          float gain_linear = std::pow(10.0f, gain / 20.0f);
          sample *= gain_linear;
        }
      }

      // Echo-Level berechnen
      echo_level += std::abs(sample);
    }

    echo_level /= audio_data.size();
    return echo_level;
  }
};

// ── EchoCancellation ──────────────────────────────────────────────────

bool EchoCancellation::initialize(int sample_rate, int channels) {
  if (is_initialized_) {
    return true;
  }

  impl_ = std::make_unique<Impl>();
  if (!impl_->initialize(sample_rate, channels)) {
    return false;
  }

  is_initialized_ = true;
  sample_rate_ = sample_rate;
  channels_ = channels;

  hh::get_logger().info("EchoCancellation: Created (rate={}, channels={})", sample_rate, channels);
  return true;
}

void EchoCancellation::shutdown() {
  if (!is_initialized_) {
    return;
  }

  impl_->shutdown();
  impl_.reset();
  is_initialized_ = false;

  hh::get_logger().info("EchoCancellation: Destroyed");
}

EchoCancellation::ProcessResult EchoCancellation::process_capture(std::vector<float>& audio_data) {
  ProcessResult result{};

  if (!is_initialized_ || !impl_) {
    return result;
  }

  float echo_level = impl_->process_audio(audio_data, true);
  result.echo_removed = true; // Simuliert
  result.echo_level_db = (echo_level > 0.0f) ? 20.0f * std::log10(echo_level) : -100.0f;
  result.residual_echo = (result.echo_level_db > -40.0f);

  stats_.frames_processed++;
  if (result.echo_removed) {
    stats_.echo_removed_count++;
  }
  if (result.residual_echo) {
    stats_.residual_echo_count++;
  }
  stats_.average_echo_level_db = (stats_.average_echo_level_db * (stats_.frames_processed - 1) + result.echo_level_db) / stats_.frames_processed;

  if (stats_callback_) {
    stats_callback_(stats_);
  }

  return result;
}

EchoCancellation::ProcessResult EchoCancellation::process_playback(std::vector<float>& audio_data) {
  ProcessResult result{};

  if (!is_initialized_ || !impl_) {
    return result;
  }

  float echo_level = impl_->process_audio(audio_data, false);
  result.echo_removed = true; // Simuliert
  result.echo_level_db = (echo_level > 0.0f) ? 20.0f * std::log10(echo_level) : -100.0f;
  result.residual_echo = (result.echo_level_db > -40.0f);

  stats_.frames_processed++;
  if (result.echo_removed) {
    stats_.echo_removed_count++;
  }
  if (result.residual_echo) {
    stats_.residual_echo_count++;
  }
  stats_.average_echo_level_db = (stats_.average_echo_level_db * (stats_.frames_processed - 1) + result.echo_level_db) / stats_.frames_processed;

  if (stats_callback_) {
    stats_callback_(stats_);
  }

  return result;
}

bool EchoCancellation::set_echo_delay_ms(int delay_ms) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  impl_->echo_delay_ms_ = delay_ms;
  hh::get_logger().info("EchoCancellation: Echo delay set to {}ms", delay_ms);
  return true;
}

bool EchoCancellation::set_agc(bool enabled, float target_level_dbfs, float gain_db) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  impl_->agc_enabled_ = enabled;
  impl_->agc_target_level_dbfs_ = target_level_dbfs;
  impl_->agc_gain_db_ = gain_db;
  hh::get_logger().info("EchoCancellation: AGC {} (target: {:.1f}dBFS, gain: {:.1f}dB)",
                        enabled ? "enabled" : "disabled", target_level_dbfs, gain_db);
  return true;
}

bool EchoCancellation::set_high_pass_filter(bool enabled) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  impl_->high_pass_enabled_ = enabled;
  hh::get_logger().info("EchoCancellation: High-pass filter {}", enabled ? "enabled" : "disabled");
  return true;
}

EchoCancellation::Stats EchoCancellation::get_stats() const {
  return stats_;
}

void EchoCancellation::reset_stats() {
  stats_ = {};
  hh::get_logger().info("EchoCancellation: Stats reset");
}

void EchoCancellation::set_stats_callback(StatsCallback callback) {
  stats_callback_ = callback;
}

} // namespace hh::client

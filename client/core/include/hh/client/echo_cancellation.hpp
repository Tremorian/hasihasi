// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace hh::client {

// Echo Cancellation mit WebRTC AudioProcessing
class EchoCancellation {
public:
  EchoCancellation() = default;
  ~EchoCancellation() = default;

  // Initialisierung
  bool initialize(int sample_rate = 48000, int channels = 1);
  void shutdown();
  bool is_initialized() const { return is_initialized_; }

  // Echo-Cancellation anwenden
  struct ProcessResult {
    bool echo_removed;
    bool residual_echo;
    float echo_level_db;
  };

  ProcessResult process_capture(std::vector<float>& audio_data);
  ProcessResult process_playback(std::vector<float>& audio_data);

  // Konfiguration
  bool set_echo_delay_ms(int delay_ms);
  bool set_agc(bool enabled, float target_level_dbfs = -31.0f, float gain_db = 0.0f);
  bool set_high_pass_filter(bool enabled);

  // Statistik
  struct Stats {
    int frames_processed;
    int echo_removed_count;
    float average_echo_level_db;
    int residual_echo_count;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using StatsCallback = std::function<void(const Stats&)>;
  void set_stats_callback(StatsCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool is_initialized_ = false;

  Stats stats_;
  StatsCallback stats_callback_;
  int sample_rate_ = 48000;
  int channels_ = 1;
};

} // namespace hh::client

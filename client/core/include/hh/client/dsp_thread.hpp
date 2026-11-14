// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "hh/audio/buffers.hpp"
#include "hh/audio/noise_suppression.hpp"
#include "hh/audio/vad.hpp"

#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <span>
#include <vector>

namespace hh::client {

// DSP-Thread für Audio-Verarbeitung
// Pipeline: Highpass → RNNoise → AGC → VAD
// Highpass und AGC sind Platzhalter, die später implementiert werden
class DspThread {
public:
  DspThread();
  ~DspThread();

  // Nicht kopierbar, bewegbar
  DspThread(const DspThread&) = delete;
  DspThread& operator=(const DspThread&) = delete;
  DspThread(DspThread&&) noexcept;
  DspThread& operator=(DspThread&&) noexcept;

  // DSP-Thread starten/stoppen
  bool start();
  void stop();
  bool is_running() const { return running_; }

  // Audio-Verarbeitung
  void process_audio(std::span<const std::int16_t> input, std::vector<std::int16_t>& output);
  
  // Callback für verarbeitetes Audio
  using AudioCallback = std::function<void(const std::vector<std::int16_t>&)>;
  void set_audio_callback(AudioCallback callback);

  // Parameter-Konfiguration
  void set_highpass_cutoff(float cutoff_hz);
  void set_noise_suppression(int level);
  void set_agc_target_level(int target_level_dbfs);
  void set_agc_compression_gain_db(float compression_gain_db);
  void set_vad_threshold(float threshold);
  void set_vad_speech_threshold(float speech_threshold);
  void set_vad_noise_threshold(float noise_threshold);

  // Status-Abfragen
  bool is_speaking() const { return vad_is_speaking_; }
  float get_input_level() const { return input_level_; }
  float get_output_level() const { return output_level_; }

  // DSP-Statistiken
  struct DspStats {
    float input_level = 0.0f;
    float output_level = 0.0f;
    bool is_speaking = false;
    float noise_suppression_level = 0.0f;
    float agc_gain_db = 0.0f;
    uint32_t processed_frames = 0;
    uint32_t dropped_frames = 0;
  };

  DspStats get_stats() const;

  // Reset-Statistiken
  void reset_stats();

private:
  void run();
  void process_pipeline(std::span<const std::int16_t> input, std::vector<std::int16_t>& output);

  // DSP-Komponenten
  std::unique_ptr<hh::audio::HighpassFilter> highpass_;
  std::unique_ptr<hh::audio::NoiseSuppression> noise_suppression_;
  std::unique_ptr<hh::audio::AutomaticGainControl> agc_;
  std::unique_ptr<hh::audio::VoiceActivityDetector> vad_;

  // Thread
  std::thread thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stop_requested_{false};

  // Callback
  AudioCallback audio_callback_;

  // Status
  std::atomic<bool> vad_is_speaking_{false};
  std::atomic<float> input_level_{0.0f};
  std::atomic<float> output_level_{0.0f};

  // Statistiken
  mutable std::atomic<uint32_t> processed_frames_{0};
  mutable std::atomic<uint32_t> dropped_frames_{0};

  // Parameter
  float highpass_cutoff_hz_ = 100.0f;
  int noise_suppression_level_ = 1;
  int agc_target_level_dbfs_ = -25;
  float agc_compression_gain_db_ = 25.0f;
  float vad_threshold_ = 0.5f;
  float vad_speech_threshold_ = 0.5f;
  float vad_noise_threshold_ = 0.3f;
};

} // namespace hh::client

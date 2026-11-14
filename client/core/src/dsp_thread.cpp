// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/dsp_thread.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cmath>

namespace hh::client {

// ── DspThread ─────────────────────────────────────────────────────────

DspThread::DspThread() {
  // DSP-Komponenten initialisieren
  highpass_ = std::make_unique<hh::audio::HighpassFilter>();
  noise_suppression_ = std::make_unique<hh::audio::NoiseSuppression>();
  agc_ = std::make_unique<hh::audio::AutomaticGainControl>();
  vad_ = std::make_unique<hh::audio::VoiceActivityDetector>();
  
  hh::get_logger().info("DSP: Thread initialized");
}

DspThread::~DspThread() {
  stop();
  hh::get_logger().info("DSP: Thread destroyed");
}

DspThread::DspThread(DspThread&& other) noexcept
  : thread_(std::move(other.thread_))
  , running_(other.running_.load())
  , stop_requested_(other.stop_requested_.load())
  , audio_callback_(std::move(other.audio_callback_))
  , vad_is_speaking_(other.vad_is_speaking_.load())
  , input_level_(other.input_level_.load())
  , output_level_(other.output_level_.load())
  , processed_frames_(other.processed_frames_.load())
  , dropped_frames_(other.dropped_frames_.load())
  , highpass_cutoff_hz_(other.highpass_cutoff_hz_)
  , noise_suppression_level_(other.noise_suppression_level_)
  , agc_target_level_dbfs_(other.agc_target_level_dbfs_)
  , agc_compression_gain_db_(other.agc_compression_gain_db_)
  , vad_threshold_(other.vad_threshold_)
  , vad_speech_threshold_(other.vad_speech_threshold_)
  , vad_noise_threshold_(other.vad_noise_threshold_) {
  
  other.running_ = false;
  other.stop_requested_ = true;
}

DspThread& DspThread::operator=(DspThread&& other) noexcept {
  if (this != &other) {
    stop();
    
    thread_ = std::move(other.thread_);
    running_ = other.running_.load();
    stop_requested_ = other.stop_requested_.load();
    audio_callback_ = std::move(other.audio_callback_);
    vad_is_speaking_ = other.vad_is_speaking_.load();
    input_level_ = other.input_level_.load();
    output_level_ = other.output_level_.load();
    processed_frames_ = other.processed_frames_.load();
    dropped_frames_ = other.dropped_frames_.load();
    highpass_cutoff_hz_ = other.highpass_cutoff_hz_;
    noise_suppression_level_ = other.noise_suppression_level_;
    agc_target_level_dbfs_ = other.agc_target_level_dbfs_;
    agc_compression_gain_db_ = other.agc_compression_gain_db_;
    vad_threshold_ = other.vad_threshold_;
    vad_speech_threshold_ = other.vad_speech_threshold_;
    vad_noise_threshold_ = other.vad_noise_threshold_;
    
    other.running_ = false;
    other.stop_requested_ = true;
  }
  return *this;
}

bool DspThread::start() {
  if (running_.load()) {
    hh::get_logger().warn("DSP: Thread already running");
    return false;
  }
  
  stop_requested_ = false;
  running_ = true;
  thread_ = std::thread(&DspThread::run, this);
  
  hh::get_logger().info("DSP: Thread started");
  return true;
}

void DspThread::stop() {
  if (!running_.load()) {
    return;
  }
  
  stop_requested_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
  running_ = false;
  
  hh::get_logger().info("DSP: Thread stopped");
}

void DspThread::process_audio(std::span<const std::int16_t> input, std::vector<std::int16_t>& output) {
  if (!running_.load()) {
    hh::get_logger().warn("DSP: Thread not running, processing inline");
    process_pipeline(input, output);
    return;
  }
  
  // Async processing: copy input to internal buffer and signal thread
  // For now, we'll process inline and let the thread handle continuous processing
  process_pipeline(input, output);
}

void DspThread::set_audio_callback(AudioCallback callback) {
  audio_callback_ = std::move(callback);
}

void DspThread::set_highpass_cutoff(float cutoff_hz) {
  highpass_cutoff_hz_ = cutoff_hz;
  if (highpass_) {
    highpass_->set_cutoff_frequency(cutoff_hz);
  }
  hh::get_logger().info("DSP: Highpass cutoff set to {} Hz", cutoff_hz);
}

void DspThread::set_noise_suppression(int level) {
  noise_suppression_level_ = level;
  if (noise_suppression_) {
    noise_suppression_->set_level(level);
  }
  hh::get_logger().info("DSP: Noise suppression level set to {}", level);
}

void DspThread::set_agc_target_level(int target_level_dbfs) {
  agc_target_level_dbfs_ = target_level_dbfs;
  if (agc_) {
    agc_->set_target_level_dbfs(target_level_dbfs);
  }
  hh::get_logger().info("DSP: AGC target level set to {} dBFS", target_level_dbfs);
}

void DspThread::set_agc_compression_gain_db(float compression_gain_db) {
  agc_compression_gain_db_ = compression_gain_db;
  if (agc_) {
    agc_->set_compression_gain_db(compression_gain_db);
  }
  hh::get_logger().info("DSP: AGC compression gain set to {} dB", compression_gain_db);
}

void DspThread::set_vad_threshold(float threshold) {
  vad_threshold_ = threshold;
  if (vad_) {
    vad_->set_threshold(threshold);
  }
  hh::get_logger().info("DSP: VAD threshold set to {}", threshold);
}

void DspThread::set_vad_speech_threshold(float speech_threshold) {
  vad_speech_threshold_ = speech_threshold;
  if (vad_) {
    vad_->set_speech_threshold(speech_threshold);
  }
  hh::get_logger().info("DSP: VAD speech threshold set to {}", speech_threshold);
}

void DspThread::set_vad_noise_threshold(float noise_threshold) {
  vad_noise_threshold_ = noise_threshold;
  if (vad_) {
    vad_->set_noise_threshold(noise_threshold);
  }
  hh::get_logger().info("DSP: VAD noise threshold set to {}", noise_threshold);
}

DspThread::DspStats DspThread::get_stats() const {
  DspStats stats;
  stats.input_level = input_level_.load();
  stats.output_level = output_level_.load();
  stats.is_speaking = vad_is_speaking_.load();
  stats.noise_suppression_level = static_cast<float>(noise_suppression_level_);
  stats.agc_gain_db = agc_compression_gain_db_;
  stats.processed_frames = processed_frames_.load();
  stats.dropped_frames = dropped_frames_.load();
  return stats;
}

void DspThread::reset_stats() {
  processed_frames_ = 0;
  dropped_frames_ = 0;
  input_level_ = 0.0f;
  output_level_ = 0.0f;
  vad_is_speaking_ = false;
}

void DspThread::run() {
  hh::get_logger().info("DSP: Thread running");
  
  while (!stop_requested_.load()) {
    // Hier würde die kontinuierliche Audio-Verarbeitung stattfinden
    // Für jetzt: Sleep und prüfen, ob gestoppt werden soll
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  
  hh::get_logger().info("DSP: Thread stopped");
}

void DspThread::process_pipeline(std::span<const std::int16_t> input, std::vector<std::int16_t>& output) {
  if (input.empty()) {
    return;
  }
  
  // Output vorbereiten
  output.resize(input.size());
  
  // Input Level berechnen
  float max_val = 0.0f;
  for (const auto& sample : input) {
    float val = std::abs(static_cast<float>(sample));
    if (val > max_val) {
      max_val = val;
    }
  }
  input_level_ = max_val / 32768.0f;
  
  // Pipeline: Highpass → RNNoise → AGC → VAD
  
  // 1. Highpass-Filter (Platzhalter)
  // highpass_->process(input, output);
  std::copy(input.begin(), input.end(), output.begin());
  
  // 2. RNNoise Rauschunterdrückung
  if (noise_suppression_) {
    noise_suppression_->process(output.data(), output.size());
  }
  
  // 3. AGC (Automatic Gain Control)
  if (agc_) {
    agc_->process(output.data(), output.size());
  }
  
  // 4. VAD (Voice Activity Detection)
  if (vad_) {
    vad_is_speaking_ = vad_->is_speaking(output.data(), output.size());
  }
  
  // Output Level berechnen
  max_val = 0.0f;
  for (const auto& sample : output) {
    float val = std::abs(static_cast<float>(sample));
    if (val > max_val) {
      max_val = val;
    }
  }
  output_level_ = max_val / 32768.0f;
  
  // Callback aufrufen
  if (audio_callback_) {
    audio_callback_(output);
  }
  
  // Statistiken aktualisieren
  processed_frames_++;
}

} // namespace hh::client

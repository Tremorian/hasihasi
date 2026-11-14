// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/voice_pipeline.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cmath>

namespace hh::client {

VoicePipeline::VoicePipeline(const Config& config)
  : config_(config)
  , capture_buffer_(config_.sample_rate * config_.frame_size_ms / 1000)
  , playback_buffer_(config_.sample_rate * config_.frame_size_ms / 1000)
  , vad_(hh::audio::VoiceActivityDetector::Config{
      config_.sample_rate * config_.frame_size_ms / 1000,
      config_.sample_rate,
      config_.vad_threshold,
      10.0,
      3,
      5,
      300
    })
  , jitter_buffer_(hh::audio::JitterBufferConfig{
      40, 20, 120, 2, 5, 3,
      config_.sample_rate,
      config_.frame_size_ms
    }) {
  hh::get_logger().info("VoicePipeline initialized: sample_rate={}, frame_size={}ms, bitrate={}",
                        config_.sample_rate, config_.frame_size_ms, config_.bitrate_bps);
}

VoicePipeline::~VoicePipeline() {
  stop();
}

VoicePipeline::VoicePipeline(VoicePipeline&&) noexcept = default;
VoicePipeline& VoicePipeline::operator=(VoicePipeline&&) noexcept = default;

bool VoicePipeline::start(std::shared_ptr<Session> session) {
  if (running_) {
    hh::get_logger().warn("VoicePipeline already running");
    return false;
  }

  session_ = session;

  // PipeWire-Capture öffnen
  if (!capture_.open(config_.capture_device)) {
    hh::get_logger().error("Failed to open PipeWire capture");
    return false;
  }

  // PipeWire-Playback öffnen
  if (!playback_.open(config_.playback_device)) {
    hh::get_logger().error("Failed to open PipeWire playback");
    return false;
  }

  // Opus-Encoder initialisieren
  if (!encoder_.initialize(config_.sample_rate, config_.channels, config_.frame_size_ms)) {
    hh::get_logger().error("Failed to initialize Opus encoder");
    return false;
  }

  // Opus-Decoder initialisieren
  if (!decoder_.initialize(config_.sample_rate, config_.channels)) {
    hh::get_logger().error("Failed to initialize Opus decoder");
    return false;
  }

  // Noise Suppressor initialisieren
  if (config_.enable_noise_suppression) {
    noise_suppressor_.initialize(hh::audio::NoiseSuppressor::Config{
      config_.sample_rate,
      config_.sample_rate * config_.frame_size_ms / 1000,
      config_.noise_suppression_strength
    });
  }

  // Capture-Callback setzen
  capture_.set_callback([this](std::span<const int16_t> pcm) {
    if (!running_ || muted_) return;

    // Capture-Buffer füllen
    capture_buffer_.assign(pcm.begin(), pcm.end());

    // VAD prüfen
    bool speaking = check_vad(capture_buffer_);
    is_speaking_ = speaking;

    // Volumen berechnen
    current_volume_ = calculate_volume(capture_buffer_);

    // Audio-Status-Callback
    if (audio_status_callback_) {
      audio_status_callback_(current_volume_, is_speaking_);
    }

    // Noise Suppression
    std::vector<int16_t> processed = capture_buffer_;
    if (config_.enable_noise_suppression) {
      processed = noise_suppressor_.suppress(capture_buffer_);
    }

    // Opus-Encoder
    auto encoded = encoder_.encode(processed);

    // Frame zählen
    encoded_frames_++;

    // An Session senden
    {
      std::lock_guard<std::mutex> lock(session_mutex_);
      if (session_ && session_->is_connected()) {
        session_->send_voice_data(encoded);
      }
    }

    // Voice-Data-Callback
    if (voice_data_callback_) {
      voice_data_callback_(encoded);
    }
  });

  // Playback-Callback setzen
  playback_.set_buffer_callback([this](std::span<int16_t> buffer) {
    if (!running_) {
      buffer.fill(0);
      return;
    }

    // Jitter-Buffer holen
    auto decoded = jitter_buffer_.get();
    if (decoded.has_value()) {
      decoded_frames_++;
      std::copy(decoded->begin(), decoded->end(), buffer.begin());
    } else {
      // PLC: Stille
      buffer.fill(0);
    }
  });

  // Starten
  capture_.start();
  playback_.start();
  running_ = true;

  // Threads starten
  capture_thread_ = std::thread([this]() {
    capture_thread_func();
  });

  playback_thread_ = std::thread([this]() {
    playback_thread_func();
  });

  hh::get_logger().info("VoicePipeline started");
  return true;
}

void VoicePipeline::stop() {
  if (!running_) return;

  running_ = false;

  capture_.stop();
  playback_.stop();

  if (capture_thread_.joinable()) {
    capture_thread_.join();
  }
  if (playback_thread_.joinable()) {
    playback_thread_.join();
  }

  hh::get_logger().info("VoicePipeline stopped");
}

void VoicePipeline::set_muted(bool muted) {
  muted_ = muted;
  hh::get_logger().info("VoicePipeline muted={}", muted);
}

void VoicePipeline::capture_thread_func() {
  hh::get_logger().info("Capture thread started");

  while (running_) {
    // Capture-Thread: Wird durch PipeWire-Callback getriggert
    // Hier können zusätzliche Verarbeitungen stattfinden
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  hh::get_logger().info("Capture thread stopped");
}

void VoicePipeline::playback_thread_func() {
  hh::get_logger().info("Playback thread started");

  while (running_) {
    // Playback-Thread: Wird durch PipeWire-Callback getriggert
    // Hier können zusätzliche Verarbeitungen stattfinden
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  hh::get_logger().info("Playback thread stopped");
}

bool VoicePipeline::check_vad(std::span<const int16_t> pcm) {
  return vad_.analyze_frame(pcm);
}

double VoicePipeline::calculate_volume(std::span<const int16_t> pcm) {
  double sum = 0.0;
  for (int16_t sample : pcm) {
    sum += std::abs(static_cast<double>(sample));
  }
  double avg = sum / pcm.size();
  return (avg / 32768.0) * 100.0; // 0-100%
}

} // namespace hh::client

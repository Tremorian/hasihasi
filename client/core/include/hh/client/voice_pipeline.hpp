// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "hh/audio/buffers.hpp"
#include "hh/audio/pipewire.hpp"
#include "hh/client/session.hpp"

namespace hh::client {

// Voice-Pipeline: PipeWire → Opus → Netzwerk → Opus → PipeWire
class VoicePipeline {
public:
  using VoiceDataCallback = std::function<void(std::span<const uint8_t>)>;
  using AudioStatusCallback = std::function<void(double volume_percent, bool is_speaking)>;

  struct Config {
    std::string capture_device = "";      // Mikrofon
    std::string playback_device = "";     // Lautsprecher
    int sample_rate = 48000;
    int channels = 1;
    int frame_size_ms = 10;
    int bitrate_bps = 40000;              // 40 kbit/s
    bool enable_vad = true;
    bool enable_noise_suppression = true;
    float vad_threshold = 30.0f;
    float noise_suppression_strength = 0.8f;
  };

  explicit VoicePipeline(const Config& config = Config{});
  ~VoicePipeline();

  // Nicht kopierbar, bewegbar
  VoicePipeline(const VoicePipeline&) = delete;
  VoicePipeline& operator=(const VoicePipeline&) = delete;
  VoicePipeline(VoicePipeline&&) noexcept;
  VoicePipeline& operator=(VoicePipeline&&) noexcept;

  // Pipeline starten/stoppen
  bool start(std::shared_ptr<Session> session);
  void stop();

  // Status
  bool is_running() const { return running_; }
  bool is_muted() const { return muted_; }
  void set_muted(bool muted);

  // Callbacks
  void set_voice_data_callback(VoiceDataCallback cb) { voice_data_callback_ = std::move(cb); }
  void set_audio_status_callback(AudioStatusCallback cb) { audio_status_callback_ = std::move(cb); }

  // Statistik
  double current_volume() const { return current_volume_; }
  bool is_speaking() const { return is_speaking_; }
  int encoded_frames() const { return encoded_frames_; }
  int decoded_frames() const { return decoded_frames_; }

private:
  // Capture-Thread: PipeWire → VAD → Noise Suppression → Opus → Netzwerk
  void capture_thread_func();

  // Playback-Thread: Netzwerk → Opus → PipeWire
  void playback_thread_func();

  // VAD prüfen
  bool check_vad(std::span<const int16_t> pcm);

  // Volumen berechnen
  double calculate_volume(std::span<const int16_t> pcm);

  // Konfiguration
  Config config_;

  // Audio-Komponenten
  hh::audio::PipeWireCapture capture_;
  hh::audio::PipeWirePlayback playback_;
  hh::audio::OpusEncoder encoder_;
  hh::audio::OpusDecoder decoder_;

  // VAD und Noise Suppression
  hh::audio::VoiceActivityDetector vad_;
  hh::audio::NoiseSuppressor noise_suppressor_;

  // Jitter-Buffer für eingehende Audio
  hh::audio::JitterBuffer jitter_buffer_;

  // Session
  std::shared_ptr<Session> session_;

  // Thread-Management
  std::thread capture_thread_;
  std::thread playback_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> muted_{false};

  // Status
  std::atomic<double> current_volume_{0.0};
  std::atomic<bool> is_speaking_{false};
  std::atomic<int> encoded_frames_{0};
  std::atomic<int> decoded_frames_{0};

  // Puffer für Capture/Playback
  std::vector<int16_t> capture_buffer_;
  std::vector<int16_t> playback_buffer_;

  // Mutex für Session-Zugriff
  std::mutex session_mutex_;

  // Callbacks
  VoiceDataCallback voice_data_callback_;
  AudioStatusCallback audio_status_callback_;
};

} // namespace hh::client

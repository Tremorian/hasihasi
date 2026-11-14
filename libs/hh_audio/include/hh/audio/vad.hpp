// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <deque>
#include <mutex>

namespace hh::audio {

// VAD (Voice-Activation-Detection)
// Erkennt Sprache um PTT zu ersetzen (Hands-free)
class VoiceActivityDetector {
public:
  // Konfiguration
  struct Config {
    int frame_size_samples = 480;        // Samples pro Frame (10ms @ 48kHz)
    int sample_rate = 48000;             // Abtastrate
    double energy_threshold = 30.0;      // Energieschwelle
    double zcr_threshold = 10.0;         // Null-Durchgangsrate
    int frames_to_speech = 3;            // Frames bis "Sprache" erkannt
    int frames_to_silence = 5;           // Frames bis "Stille" erkannt
    int max_speech_duration_frames = 300; // Max. Sprechdauer (3s)
  };

  explicit VoiceActivityDetector(const Config& config = Config{});

  // Frame analysieren
  // Gibt true zurück, wenn Sprache erkannt
  bool analyze_frame(std::span<const int16_t> pcm_data);

  // Aktueller Status
  enum class State {
    SILENCE,
    SPEECH
  };

  State current_state() const { return state_; }
  bool is_speaking() const { return state_ == State::SPEECH; }

  // Statistik
  double speech_probability() const { return speech_probability_; }
  double current_energy() const { return current_energy_; }
  double current_zcr() const { return current_zcr_; }

  // Reset
  void reset();

private:
  double calculate_energy(std::span<const int16_t> data) const;
  double calculate_zcr(std::span<const int16_t> data) const;
  double calculate_spectral_features(std::span<const int16_t> data) const;

  Config config_;
  State state_ = State::SILENCE;
  int speech_counter_ = 0;
  int silence_counter_ = 0;

  double speech_probability_ = 0.0;
  double current_energy_ = 0.0;
  double current_zcr_ = 0.0;

  std::deque<double> energy_history_;
  std::deque<double> zcr_history_;

  std::mutex mutex_;
};

} // namespace hh::audio

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <memory>

namespace hh::audio {

// RNNoise-Integration für Rauschunterdrückung
// Verwendet die RNNoise-Bibliothek (https://github.com/xiph/rnnoise)
class NoiseSuppressor {
public:
  // Konfiguration
  struct Config {
    int sample_rate = 48000;
    int frame_size_samples = 480;  // 10ms @ 48kHz
    float suppression_strength = 0.8f; // 0.0 = aus, 1.0 = max
  };

  NoiseSuppressor();
  ~NoiseSuppressor();

  // Nicht kopierbar, bewegbar
  NoiseSuppressor(const NoiseSuppressor&) = delete;
  NoiseSuppressor& operator=(const NoiseSuppressor&) = delete;
  NoiseSuppressor(NoiseSuppressor&&) noexcept;
  NoiseSuppressor& operator=(NoiseSuppressor&&) noexcept;

  // Initialisieren (muss vor erster Nutzung aufgerufen werden)
  bool initialize(const Config& config = Config{});

  // Rauschunterdrückung auf Frame anwenden
  // Gibt unterdrückten Frame zurück
  std::vector<int16_t> suppress(std::span<const int16_t> pcm_data);

  // Konfiguration aktualisieren
  void set_suppression_strength(float strength);
  float suppression_strength() const { return suppression_strength_; }

  // Statistik
  int frames_processed() const { return frames_processed_; }

private:
  struct RNNoiseState; // PIMPL für RNNoise
  std::unique_ptr<RNNoiseState> state_;

  Config config_;
  float suppression_strength_ = 0.8f;
  int frames_processed_ = 0;
};

} // namespace hh::audio

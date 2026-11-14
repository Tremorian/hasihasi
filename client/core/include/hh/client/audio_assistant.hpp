// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace hh::client {

// Audio-Assistent für erste Konfiguration
class AudioAssistant {
public:
  AudioAssistant() = default;
  ~AudioAssistant() = default;

  // Geräte-Abfrage
  struct AudioDevice {
    std::string name;
    std::string id;
    bool is_input;
    int sample_rate;
    int channels;
  };

  std::vector<AudioDevice> get_input_devices() const;
  std::vector<AudioDevice> get_output_devices() const;

  // Assistent-Flow
  enum class Step {
    SELECT_INPUT,
    SELECT_OUTPUT,
    TEST_LEVELS,
    TEST_NOISE_SUPPRESSION,
    COMPLETE
  };

  Step current_step() const { return current_step_; }
  void set_current_step(Step step);

  // Geräte-Auswahl
  bool select_input_device(const std::string& device_id);
  bool select_output_device(const std::string& device_id);

  // Pegel-Test
  struct LevelTestResult {
    bool success;
    float average_volume;
    float peak_volume;
    bool noise_detected;
  };

  LevelTestResult test_input_levels(int duration_ms = 3000);
  LevelTestResult test_output_levels(int duration_ms = 3000);

  // Rauschunterdrückung-Test
  struct NoiseSuppressionTestResult {
    bool success;
    float noise_reduction_db;
    bool artifact_detected;
  };

  NoiseSuppressionTestResult test_noise_suppression(int duration_ms = 5000);

  // Konfiguration speichern
  bool save_configuration();

  // Konfiguration laden
  bool load_configuration();

  // Assistent abgeschlossen
  bool is_complete() const { return current_step_ == Step::COMPLETE; }
  void mark_complete();

  // Callbacks
  using ProgressCallback = std::function<void(Step, const std::string&)>;
  void set_progress_callback(ProgressCallback callback);

private:
  Step current_step_ = Step::SELECT_INPUT;
  ProgressCallback progress_callback_;
  std::string selected_input_device_;
  std::string selected_output_device_;
};

} // namespace hh::client

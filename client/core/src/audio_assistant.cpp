// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/audio_assistant.hpp"

#include "hh/logging.hpp"

#include <random>
#include <chrono>
#include <cmath>

namespace hh::client {

// ── Geräte-Abfrage ────────────────────────────────────────────────────

std::vector<AudioAssistant::AudioDevice> AudioAssistant::get_input_devices() const {
  std::vector<AudioDevice> devices;

  // PipeWire-Geräte abfragen
  // In einer echten Implementierung würde dies pw-cli oder libpipewire verwenden
  devices.push_back({
    "Standard-Mikrofon",
    "input:default",
    true,
    48000,
    1
  });

  devices.push_back({
    "Headset-Mikrofon",
    "input:headset",
    true,
    48000,
    1
  });

  return devices;
}

std::vector<AudioAssistant::AudioDevice> AudioAssistant::get_output_devices() const {
  std::vector<AudioDevice> devices;

  // PipeWire-Geräte abfragen
  devices.push_back({
    "Standard-Lautsprecher",
    "output:default",
    false,
    48000,
    2
  });

  devices.push_back({
    "Headset-Lautsprecher",
    "output:headset",
    false,
    48000,
    2
  });

  return devices;
}

// ── Assistent-Flow ────────────────────────────────────────────────────

void AudioAssistant::set_current_step(Step step) {
  current_step_ = step;

  std::string step_name;
  switch (step) {
    case Step::SELECT_INPUT: step_name = "Eingabegerät wählen"; break;
    case Step::SELECT_OUTPUT: step_name = "Ausgabegerät wählen"; break;
    case Step::TEST_LEVELS: step_name = "Pegel testen"; break;
    case Step::TEST_NOISE_SUPPRESSION: step_name = "Rauschunterdrückung testen"; break;
    case Step::COMPLETE: step_name = "Abgeschlossen"; break;
  }

  hh::get_logger().info("AudioAssistant: Step changed to {}", step_name);

  if (progress_callback_) {
    progress_callback_(step, step_name);
  }
}

bool AudioAssistant::select_input_device(const std::string& device_id) {
  selected_input_device_ = device_id;
  hh::get_logger().info("AudioAssistant: Input device selected: {}", device_id);

  set_current_step(Step::SELECT_OUTPUT);
  return true;
}

bool AudioAssistant::select_output_device(const std::string& device_id) {
  selected_output_device_ = device_id;
  hh::get_logger().info("AudioAssistant: Output device selected: {}", device_id);

  set_current_step(Step::TEST_LEVELS);
  return true;
}

// ── Pegel-Test ────────────────────────────────────────────────────────

AudioAssistant::LevelTestResult AudioAssistant::test_input_levels(int duration_ms) {
  LevelTestResult result{};

  hh::get_logger().info("AudioAssistant: Testing input levels for {}ms", duration_ms);

  // Simuliere Pegel-Messung
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(0.0f, 1.0f);

  float total_volume = 0.0f;
  float peak_volume = 0.0f;
  int samples = duration_ms / 10; // 10ms Intervalle
  bool noise_detected = false;

  for (int i = 0; i < samples; i++) {
    float volume = dist(gen);
    total_volume += volume;

    if (volume > peak_volume) {
      peak_volume = volume;
    }

    // Rauschen erkennen (sehr niedrige oder sehr hohe Werte)
    if (volume < 0.05f || volume > 0.95f) {
      noise_detected = true;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  result.average_volume = total_volume / samples;
  result.peak_volume = peak_volume;
  result.success = (result.average_volume > 0.1f && result.average_volume < 0.8f);
  result.noise_detected = noise_detected;

  hh::get_logger().info("AudioAssistant: Input levels - avg: {:.2f}, peak: {:.2f}, success: {}",
                        result.average_volume, result.peak_volume, result.success);

  if (result.success) {
    set_current_step(Step::TEST_NOISE_SUPPRESSION);
  }

  return result;
}

AudioAssistant::LevelTestResult AudioAssistant::test_output_levels(int duration_ms) {
  LevelTestResult result{};

  hh::get_logger().info("AudioAssistant: Testing output levels for {}ms", duration_ms);

  // Simuliere Pegel-Messung
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(0.0f, 1.0f);

  float total_volume = 0.0f;
  float peak_volume = 0.0f;
  int samples = duration_ms / 10;

  for (int i = 0; i < samples; i++) {
    float volume = dist(gen);
    total_volume += volume;

    if (volume > peak_volume) {
      peak_volume = volume;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  result.average_volume = total_volume / samples;
  result.peak_volume = peak_volume;
  result.success = (result.average_volume > 0.0f);

  hh::get_logger().info("AudioAssistant: Output levels - avg: {:.2f}, peak: {:.2f}, success: {}",
                        result.average_volume, result.peak_volume, result.success);

  return result;
}

// ── Rauschunterdrückung-Test ──────────────────────────────────────────

AudioAssistant::NoiseSuppressionTestResult AudioAssistant::test_noise_suppression(int duration_ms) {
  NoiseSuppressionTestResult result{};

  hh::get_logger().info("AudioAssistant: Testing noise suppression for {}ms", duration_ms);

  // Simuliere Rauschunterdrückungstest
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(10.0f, 30.0f);

  // Simuliere Rauschreduzierung in dB
  float noise_reduction = dist(gen);
  result.noise_reduction_db = noise_reduction;
  result.success = (noise_reduction > 15.0f);
  result.artifact_detected = false;

  hh::get_logger().info("AudioAssistant: Noise suppression - reduction: {:.1f}dB, success: {}",
                        noise_reduction, result.success);

  if (result.success) {
    mark_complete();
  }

  return result;
}

// ── Konfiguration ─────────────────────────────────────────────────────

bool AudioAssistant::save_configuration() {
  hh::get_logger().info("AudioAssistant: Saving configuration");
  // In einer echten Implementierung würde dies die Konfiguration speichern
  return true;
}

bool AudioAssistant::load_configuration() {
  hh::get_logger().info("AudioAssistant: Loading configuration");
  // In einer echten Implementierung würde dies die Konfiguration laden
  return true;
}

void AudioAssistant::mark_complete() {
  set_current_step(Step::COMPLETE);
  hh::get_logger().info("AudioAssistant: Assistant completed");
}

void AudioAssistant::set_progress_callback(ProgressCallback callback) {
  progress_callback_ = callback;
}

} // namespace hh::client

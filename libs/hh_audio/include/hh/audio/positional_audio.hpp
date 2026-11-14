// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <cmath>
#include <span>
#include <vector>
#include <array>

namespace hh::audio {

// HRTF (Head-Related Transfer Function) für 3D-Audio
// Simuliert räumliches Hören basierend auf Position und Orientierung
class HRTFProcessor {
public:
  // Konfiguration
  struct Config {
    int sample_rate = 48000;
    int frame_size_samples = 480;  // 10ms @ 48kHz
    float speaker_distance = 1.0f; // Lautsprecher-Abstand in Metern
    float head_radius = 0.0875f;   // Kopfradius in Metern (8.75cm)
  };

  explicit HRTFProcessor(const Config& config = Config{});

  // 3D-Position eines Nutzers setzen
  void set_user_position(uint32_t user_id, float x, float y, float z);

  // Listener-Position (Spieler) setzen
  void set_listener_position(float x, float y, float z);
  void set_listener_orientation(float yaw, float pitch);

  // Audio-Frame mit HRTF verarbeiten
  // Gibt räumlich verarbeitetes Stereo-Audio zurück
  std::array<std::vector<int16_t>, 2> process(uint32_t user_id,
                                               std::span<const int16_t> mono_input);

  // Einfache Distanz-basierte Dämpfung (ohne HRTF)
  static float calculate_distance_attenuation(float distance,
                                               float reference_distance = 1.0f,
                                               float max_distance = 10.0f);

  // IR (Impulse Response) für HRTF generieren (vereinfacht)
  static std::vector<float> generate_irs(float azimuth, float elevation,
                                          float distance,
                                          int ir_length_samples);

private:
  Config config_;

  // Nutzer-Positionen
  struct UserPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
  };
  std::map<uint32_t, UserPosition> user_positions_;

  // Listener
  float listener_x_ = 0.0f;
  float listener_y_ = 0.0f;
  float listener_z_ = 0.0f;
  float listener_yaw_ = 0.0f;
  float listener_pitch_ = 0.0f;

  // Vereinfachte IRs für verschiedene Winkel
  std::vector<float> left_irs_;
  std::vector<float> right_irs_;
};

// 3D-Audio-Engine
class PositionalAudioEngine {
public:
  // Konfiguration
  struct Config {
    HRTFProcessor::Config hrtf_config;
    bool enable_hrtf = true;
    float default_volume = 1.0f;
    float min_distance = 0.5f;
    float max_distance = 10.0f;
  };

  explicit PositionalAudioEngine(const Config& config = Config{});

  // Nutzer hinzufügen
  void add_user(uint32_t user_id, const std::string& username);

  // Nutzer entfernen
  void remove_user(uint32_t user_id);

  // Position setzen
  void set_user_position(uint32_t user_id, float x, float y, float z);

  // Listener-Position setzen
  void set_listener_position(float x, float y, float z);
  void set_listener_orientation(float yaw, float pitch);

  // Audio für einen Nutzer verarbeiten
  std::array<std::vector<int16_t>, 2> process_user(uint32_t user_id,
                                                    std::span<const int16_t> mono_input,
                                                    float volume = 1.0f);

  // Alle Nutzer verarbeiten (Misch-Output)
  std::vector<int16_t> process_all(std::span<const std::pair<uint32_t, std::span<const int16_t>>> users);

  // Volumen pro Nutzer
  void set_user_volume(uint32_t user_id, float volume);

  // Mute/Deaf
  void set_user_muted(uint32_t user_id, bool muted);
  void set_user_deafened(uint32_t user_id, bool deafened);

  // Statistik
  int active_users() const { return static_cast<int>(user_positions_.size()); }

private:
  Config config_;
  HRTFProcessor hrtf_;

  struct UserData {
    std::string username;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float volume = 1.0f;
    bool muted = false;
    bool deafened = false;
  };
  std::map<uint32_t, UserData> user_positions_;
};

} // namespace hh::audio

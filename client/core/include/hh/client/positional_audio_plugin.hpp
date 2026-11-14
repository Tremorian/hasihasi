// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace hh::client {

// Positional Audio Plugin-Schnittstelle für Spiele
class PositionalAudioPlugin {
public:
  PositionalAudioPlugin() = default;
  virtual ~PositionalAudioPlugin() = default;

  // Plugin-Informationen
  struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    bool supports_positional_audio;
    bool supports_speaker_detection;
  };

  virtual PluginInfo get_info() const = 0;

  // Initialisierung
  virtual bool initialize() = 0;
  virtual void shutdown() = 0;
  virtual bool is_initialized() const = 0;

  // Positional Audio
  struct AudioSource {
    uint32_t user_id;
    std::string name;
    float position_x;
    float position_y;
    float position_z;
    float volume;
    bool is_speaking;
  };

  virtual void update_sources(const std::vector<AudioSource>& sources) = 0;
  virtual void clear_sources() = 0;

  // Speaker Detection (für Spiele)
  struct SpeakerEvent {
    uint32_t user_id;
    std::string name;
    bool is_speaking;
    float volume;
    float azimuth; // -180.0 - 180.0
    float elevation; // -90.0 - 90.0
  };

  using SpeakerCallback = std::function<void(const SpeakerEvent&)>;
  virtual void set_speaker_callback(SpeakerCallback callback) = 0;

  // Listener-Position (Spieler-Position)
  virtual void set_listener_position(float x, float y, float z) = 0;
  virtual void set_listener_orientation(float yaw, float pitch, float roll) = 0;

  // Plugin-spezifische Daten
  virtual void set_plugin_data(const std::string& key, const std::string& value) = 0;
  virtual std::string get_plugin_data(const std::string& key) const = 0;
};

// Positional Audio Plugin-Manager
class PositionalAudioPluginManager {
public:
  PositionalAudioPluginManager() = default;
  ~PositionalAudioPluginManager() = default;

  // Plugin registrieren/entfernen
  bool register_plugin(std::shared_ptr<PositionalAudioPlugin> plugin);
  bool unregister_plugin(const std::string& plugin_name);

  // Plugin abrufen
  std::shared_ptr<PositionalAudioPlugin> find_plugin(const std::string& plugin_name) const;
  std::vector<std::shared_ptr<PositionalAudioPlugin>> all_plugins() const;

  // Audio-Updates an alle Plugins senden
  void update_sources(const std::vector<PositionalAudioPlugin::AudioSource>& sources);
  void clear_sources();

  // Listener-Position an alle Plugins senden
  void set_listener_position(float x, float y, float z);
  void set_listener_orientation(float yaw, float pitch, float roll);

  // Statistik
  size_t plugin_count() const { return plugins_.size(); }

private:
  std::map<std::string, std::shared_ptr<PositionalAudioPlugin>> plugins_;
};

} // namespace hh::client

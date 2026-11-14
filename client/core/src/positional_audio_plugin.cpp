// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/positional_audio_plugin.hpp"

#include "hh/logging.hpp"

namespace hh::client {

// ── PositionalAudioPluginManager ──────────────────────────────────────

bool PositionalAudioPluginManager::register_plugin(std::shared_ptr<PositionalAudioPlugin> plugin) {
  if (!plugin) {
    hh::get_logger().error("PositionalAudioPluginManager: Null plugin");
    return false;
  }

  auto info = plugin->get_info();
  if (plugins_.find(info.name) != plugins_.end()) {
    hh::get_logger().warn("PositionalAudioPluginManager: Plugin {} already registered", info.name);
    return false;
  }

  if (!plugin->initialize()) {
    hh::get_logger().error("PositionalAudioPluginManager: Failed to initialize plugin {}", info.name);
    return false;
  }

  plugins_[info.name] = plugin;
  hh::get_logger().info("PositionalAudioPluginManager: Plugin {} registered (version {})",
                        info.name, info.version);
  return true;
}

bool PositionalAudioPluginManager::unregister_plugin(const std::string& plugin_name) {
  auto it = plugins_.find(plugin_name);
  if (it == plugins_.end()) {
    hh::get_logger().warn("PositionalAudioPluginManager: Plugin {} not found", plugin_name);
    return false;
  }

  it->second->shutdown();
  plugins_.erase(it);
  hh::get_logger().info("PositionalAudioPluginManager: Plugin {} unregistered", plugin_name);
  return true;
}

std::shared_ptr<PositionalAudioPlugin> PositionalAudioPluginManager::find_plugin(const std::string& plugin_name) const {
  auto it = plugins_.find(plugin_name);
  if (it == plugins_.end()) {
    return nullptr;
  }
  return it->second;
}

std::vector<std::shared_ptr<PositionalAudioPlugin>> PositionalAudioPluginManager::all_plugins() const {
  std::vector<std::shared_ptr<PositionalAudioPlugin>> result;
  for (const auto& [name, plugin] : plugins_) {
    result.push_back(plugin);
  }
  return result;
}

void PositionalAudioPluginManager::update_sources(const std::vector<PositionalAudioPlugin::AudioSource>& sources) {
  for (const auto& [name, plugin] : plugins_) {
    if (plugin->get_info().supports_positional_audio) {
      plugin->update_sources(sources);
    }
  }
}

void PositionalAudioPluginManager::clear_sources() {
  for (const auto& [name, plugin] : plugins_) {
    plugin->clear_sources();
  }
}

void PositionalAudioPluginManager::set_listener_position(float x, float y, float z) {
  for (const auto& [name, plugin] : plugins_) {
    plugin->set_listener_position(x, y, z);
  }
}

void PositionalAudioPluginManager::set_listener_orientation(float yaw, float pitch, float roll) {
  for (const auto& [name, plugin] : plugins_) {
    plugin->set_listener_orientation(yaw, pitch, roll);
  }
}

} // namespace hh::client

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <memory>

namespace hh::client {

// Globale Hotkey-Verwaltung für PTT und andere Aktionen
class GlobalHotkeyManager {
public:
  GlobalHotkeyManager() = default;
  ~GlobalHotkeyManager();

  // Hotkey registrieren
  bool register_hotkey(uint32_t key_code, const std::string& modifiers,
                       const std::function<void()>& callback);

  // Hotkey entfernen
  bool unregister_hotkey(uint32_t key_code, const std::string& modifiers);

  // Hotkey-Status
  bool is_registered(uint32_t key_code, const std::string& modifiers) const;

  // Plattform-spezifische Methoden
  static bool is_wayland() { return get_display_server() == DisplayServer::WAYLAND; }
  static bool is_x11() { return get_display_server() == DisplayServer::X11; }
  static DisplayServer get_display_server();

  // Initialisierung/Freigabe
  bool initialize();
  void shutdown();
  bool is_initialized() const { return is_initialized_; }

private:
  enum class DisplayServer {
    UNKNOWN,
    WAYLAND,
    X11
  };

  static DisplayServer get_display_server_internal();

  // Plattform-Implementierung
  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool is_initialized_ = false;
};

} // namespace hh::client

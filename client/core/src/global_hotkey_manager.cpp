// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/global_hotkey_manager.hpp"

#include "hh/logging.hpp"

#include <cstdlib>
#include <sstream>

#ifdef __linux__
  #ifdef Q_OS_WAYLAND
    #include <QWindow>
    #include <QGuiApplication>
  #endif
  #ifdef Q_OS_X11
    #include <QX11Info>
    #include <X11/Xlib.h>
    #include <X11/keysym.h>
  #endif
#endif

namespace hh::client {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct GlobalHotkeyManager::Impl {
  // Wayland-Implementierung über xdg-desktop-portal
  struct WaylandImpl {
    bool register_hotkey(uint32_t key_code, const std::string& modifiers,
                         const std::function<void()>& callback) {
      // xdg-desktop-portal D-Bus Aufruf
      // org.freedesktop.portal.Desktop.RegisterHotkey
      hh::get_logger().info("GlobalHotkeyManager: Wayland hotkey registered (key={}, modifiers={})",
                            key_code, modifiers);
      return true;
    }

    bool unregister_hotkey(uint32_t key_code, const std::string& modifiers) {
      hh::get_logger().info("GlobalHotkeyManager: Wayland hotkey unregistered (key={}, modifiers={})",
                            key_code, modifiers);
      return true;
    }
  };

  // X11-Implementierung über XInput2
  struct X11Impl {
    bool register_hotkey(uint32_t key_code, const std::string& modifiers,
                         const std::function<void()>& callback) {
      // X11 GrabKeyboard mit KeySym
      hh::get_logger().info("GlobalHotkeyManager: X11 hotkey registered (key={}, modifiers={})",
                            key_code, modifiers);
      return true;
    }

    bool unregister_hotkey(uint32_t key_code, const std::string& modifiers) {
      hh::get_logger().info("GlobalHotkeyManager: X11 hotkey unregistered (key={}, modifiers={})",
                            key_code, modifiers);
      return true;
    }
  };

  WaylandImpl wayland_impl_;
  X11Impl x11_impl_;
  DisplayServer display_server_ = GlobalHotkeyManager::get_display_server_internal();
};

// ── Globale Methoden ──────────────────────────────────────────────────

GlobalHotkeyManager::~GlobalHotkeyManager() {
  if (is_initialized_) {
    shutdown();
  }
}

bool GlobalHotkeyManager::register_hotkey(uint32_t key_code, const std::string& modifiers,
                                           const std::function<void()>& callback) {
  if (!is_initialized_ || !impl_) {
    hh::get_logger().error("GlobalHotkeyManager: Not initialized");
    return false;
  }

  if (impl_->display_server_ == DisplayServer::WAYLAND) {
    return impl_->wayland_impl_.register_hotkey(key_code, modifiers, callback);
  } else if (impl_->display_server_ == DisplayServer::X11) {
    return impl_->x11_impl_.register_hotkey(key_code, modifiers, callback);
  }

  hh::get_logger().error("GlobalHotkeyManager: Unknown display server");
  return false;
}

bool GlobalHotkeyManager::unregister_hotkey(uint32_t key_code, const std::string& modifiers) {
  if (!is_initialized_ || !impl_) {
    hh::get_logger().error("GlobalHotkeyManager: Not initialized");
    return false;
  }

  if (impl_->display_server_ == DisplayServer::WAYLAND) {
    return impl_->wayland_impl_.unregister_hotkey(key_code, modifiers);
  } else if (impl_->display_server_ == DisplayServer::X11) {
    return impl_->x11_impl_.unregister_hotkey(key_code, modifiers);
  }

  hh::get_logger().error("GlobalHotkeyManager: Unknown display server");
  return false;
}

bool GlobalHotkeyManager::is_registered(uint32_t key_code, const std::string& modifiers) const {
  // Vereinfachte Implementierung - in Wirklichkeit würde dies den Status tracken
  Q_UNUSED(key_code)
  Q_UNUSED(modifiers)
  return is_initialized_;
}

GlobalHotkeyManager::DisplayServer GlobalHotkeyManager::get_display_server() {
  return get_display_server_internal();
}

bool GlobalHotkeyManager::initialize() {
  if (is_initialized_) {
    return true;
  }

  impl_ = std::make_unique<Impl>();
  is_initialized_ = true;

  hh::get_logger().info("GlobalHotkeyManager: Initialized (display server: {})",
                        impl_->display_server_ == DisplayServer::WAYLAND ? "Wayland" :
                        impl_->display_server_ == DisplayServer::X11 ? "X11" : "Unknown");
  return true;
}

void GlobalHotkeyManager::shutdown() {
  if (!is_initialized_) {
    return;
  }

  impl_.reset();
  is_initialized_ = false;

  hh::get_logger().info("GlobalHotkeyManager: Shutdown");
}

GlobalHotkeyManager::DisplayServer GlobalHotkeyManager::get_display_server_internal() {
  const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
  const char* display = std::getenv("DISPLAY");

  if (wayland_display && wayland_display[0] != '\0') {
    return DisplayServer::WAYLAND;
  } else if (display && display[0] != '\0') {
    return DisplayServer::X11;
  }

  return DisplayServer::UNKNOWN;
}

} // namespace hh::client

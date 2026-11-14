// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <functional>

namespace hh::client {

// System-Tray mit Kontextmenü
class SystemTray {
public:
  SystemTray() = default;
  ~SystemTray() = default;

  // Initialisierung
  bool initialize();
  void shutdown();
  bool is_initialized() const { return is_initialized_; }

  // Menü-Actions
  void set_action_connect(const std::function<void()>& callback);
  void set_action_disconnect(const std::function<void()>& callback);
  void set_action_mute(const std::function<void()>& callback);
  void set_action_settings(const std::function<void()>& callback);
  void set_action_quit(const std::function<void()>& callback);

  // Status
  void set_status(const std::string& status);
  void set_connected(bool connected);

  // Sichtbarkeit
  void show();
  void hide();
  void show_message(const std::string& title, const std::string& message);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool is_initialized_ = false;
};

} // namespace hh::client

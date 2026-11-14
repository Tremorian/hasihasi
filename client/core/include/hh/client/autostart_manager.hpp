// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <filesystem>
#include <functional>

namespace hh::client {

// Autostart-Manager für XDG Autostart
class AutostartManager {
public:
  AutostartManager() = default;
  ~AutostartManager() = default;

  // Autostart aktivieren/deaktivieren
  bool enable_autostart();
  bool disable_autostart();
  
  // Status prüfen
  bool is_enabled() const;
  
  // Konfiguration
  void set_application_name(const std::string& name);
  void set_executable_path(const std::string& path);
  void set_hidden(bool hidden);
  
  // Statistik
  struct Stats {
    bool autostart_enabled;
    std::string autostart_file_path;
    bool is_hidden;
  };
  
  Stats get_stats() const;

  // Callbacks
  using AutostartChangedCallback = std::function<void(bool enabled)>;
  void set_autostart_changed_callback(AutostartChangedCallback callback);

private:
  std::string application_name_;
  std::string executable_path_;
  bool hidden_;
  AutostartChangedCallback autostart_changed_callback_;
  
  std::string get_autostart_dir() const;
  std::string get_autostart_file_path() const;
  bool create_autostart_file() const;
  bool remove_autostart_file() const;
  bool autostart_file_exists() const;
};

} // namespace hh::client

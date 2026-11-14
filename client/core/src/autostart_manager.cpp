// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/autostart_manager.hpp"
#include "hh/logging.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace hh::client {

bool AutostartManager::enable_autostart() {
  if (application_name_.empty()) {
    application_name_ = "HasiHasi";
  }
  if (executable_path_.empty()) {
    executable_path_ = "/usr/bin/hasihasi";
  }

  bool created = create_autostart_file();
  
  if (created) {
    hh::get_logger().info("Autostart: Enabled for {}", application_name_);
    if (autostart_changed_callback_) {
      autostart_changed_callback_(true);
    }
  } else {
    hh::get_logger().error("Autostart: Failed to enable");
  }

  return created;
}

bool AutostartManager::disable_autostart() {
  bool removed = remove_autostart_file();
  
  if (removed) {
    hh::get_logger().info("Autostart: Disabled for {}", application_name_);
    if (autostart_changed_callback_) {
      autostart_changed_callback_(false);
    }
  } else {
    hh::get_logger().debug("Autostart: Already disabled or file not found");
  }

  return removed;
}

bool AutostartManager::is_enabled() const {
  return autostart_file_exists();
}

void AutostartManager::set_application_name(const std::string& name) {
  application_name_ = name;
}

void AutostartManager::set_executable_path(const std::string& path) {
  executable_path_ = path;
}

void AutostartManager::set_hidden(bool hidden) {
  hidden_ = hidden;
}

AutostartManager::Stats AutostartManager::get_stats() const {
  Stats stats;
  stats.autostart_enabled = autostart_file_exists();
  stats.autostart_file_path = get_autostart_file_path();
  stats.is_hidden = hidden_;
  return stats;
}

void AutostartManager::set_autostart_changed_callback(AutostartChangedCallback callback) {
  autostart_changed_callback_ = std::move(callback);
}

std::string AutostartManager::get_autostart_dir() const {
  // XDG Autostart Directory
  const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
  if (xdg_data_home) {
    return std::string(xdg_data_home) + "/autostart/";
  }
  
  // ~/.config/autostart/ als Fallback
  const char* home = std::getenv("HOME");
  if (home) {
    return std::string(home) + "/.config/autostart/";
  }
  
  return "";
}

std::string AutostartManager::get_autostart_file_path() const {
  std::string dir = get_autostart_dir();
  if (dir.empty()) {
    return "";
  }
  return dir + application_name_ + ".desktop";
}

bool AutostartManager::create_autostart_file() const {
  std::string file_path = get_autostart_file_path();
  if (file_path.empty()) {
    return false;
  }

  // Directory erstellen, falls nicht existiert
  std::filesystem::path dir = std::filesystem::path(file_path).parent_path();
  std::filesystem::create_directories(dir);

  // .desktop-Datei erstellen
  std::ofstream file(file_path);
  if (!file.is_open()) {
    hh::get_logger().error("Autostart: Failed to create file: {}", file_path);
    return false;
  }

  file << "[Desktop Entry]\n";
  file << "Type=Application\n";
  file << "Name=" << application_name_ << "\n";
  file << "Exec=" << executable_path_ << "\n";
  file << "Hidden=" << (hidden_ ? "true" : "false") << "\n";
  file << "NoDisplay=" << (hidden_ ? "true" : "false") << "\n";
  file << "X-GNOME-Autostart-enabled=true\n";
  file << "X-KDE-Autostart-enabled=true\n";
  file << "Categories=Network;VoiceChat;\n";
  file << "StartupNotify=false\n";
  file << "Terminal=false\n";

  file.close();
  return true;
}

bool AutostartManager::remove_autostart_file() const {
  std::string file_path = get_autostart_file_path();
  if (file_path.empty()) {
    return false;
  }

  if (!std::filesystem::exists(file_path)) {
    return false;
  }

  std::error_code ec;
  std::filesystem::remove(file_path, ec);
  
  if (ec) {
    hh::get_logger().error("Autostart: Failed to remove file: {}", file_path);
    return false;
  }

  return true;
}

bool AutostartManager::autostart_file_exists() const {
  std::string file_path = get_autostart_file_path();
  if (file_path.empty()) {
    return false;
  }

  return std::filesystem::exists(file_path);
}

} // namespace hh::client

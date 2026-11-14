// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace hh::server {

// Server-Passwort-Authentifizierung für Gast-Zugang
class ServerPasswordAuth {
public:
  ServerPasswordAuth() = default;
  ~ServerPasswordAuth() = default;

  // Nicht kopierbar, bewegbar
  ServerPasswordAuth(const ServerPasswordAuth&) = delete;
  ServerPasswordAuth& operator=(const ServerPasswordAuth&) = delete;
  ServerPasswordAuth(ServerPasswordAuth&&) noexcept;
  ServerPasswordAuth& operator=(ServerPasswordAuth&&) noexcept;

  // Passwort setzen
  void set_password(const std::string& password);
  void clear_password();

  // Passwort prüfen
  bool verify_password(const std::string& password) const;

  // Konfiguration
  void set_enabled(bool enabled);
  bool is_enabled() const { return enabled_; }

  // Passwort-Hinting (optional, für UI)
  void set_hint(const std::string& hint);
  std::string get_hint() const { return hint_; }

  // Statistik
  struct Stats {
    size_t password_attempts = 0;
    size_t password_success = 0;
    size_t password_failures = 0;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using PasswordAttemptCallback = std::function<void(bool success, const std::string& ip)>;
  void set_password_attempt_callback(PasswordAttemptCallback callback);

private:
  bool enabled_ = false;
  std::string password_;
  std::string hint_;
  Stats stats_;
  PasswordAttemptCallback password_attempt_callback_;
  mutable std::mutex mutex_;
};

} // namespace hh::server

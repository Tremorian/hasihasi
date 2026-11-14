// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server_password_auth.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cstring>

namespace hh::server {

// ── ServerPasswordAuth ──────────────────────────────────────────────

ServerPasswordAuth::ServerPasswordAuth(ServerPasswordAuth&& other) noexcept
  : enabled_(other.enabled_)
  , password_(std::move(other.password_))
  , hint_(std::move(other.hint_))
  , stats_(other.stats_)
  , password_attempt_callback_(std::move(other.password_attempt_callback_)) {
  
  other.enabled_ = false;
  other.stats_ = Stats{};
}

ServerPasswordAuth& ServerPasswordAuth::operator=(ServerPasswordAuth&& other) noexcept {
  if (this != &other) {
    enabled_ = other.enabled_;
    password_ = std::move(other.password_);
    hint_ = std::move(other.hint_);
    stats_ = other.stats_;
    password_attempt_callback_ = std::move(other.password_attempt_callback_);
    other.enabled_ = false;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Passwort setzen ─────────────────────────────────────────────────

void ServerPasswordAuth::set_password(const std::string& password) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (password.empty()) {
    hh::get_logger().warn("ServerPasswordAuth: Cannot set empty password");
    return;
  }
  
  password_ = password;
  enabled_ = true;
  
  hh::get_logger().info("ServerPasswordAuth: Password set");
}

void ServerPasswordAuth::clear_password() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  password_.clear();
  enabled_ = false;
  
  hh::get_logger().info("ServerPasswordAuth: Password cleared");
}

// ── Passwort prüfen ─────────────────────────────────────────────────

bool ServerPasswordAuth::verify_password(const std::string& password) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  stats_.password_attempts++;
  
  if (password_ == password) {
    stats_.password_success++;
    hh::get_logger().info("ServerPasswordAuth: Password verification successful");
    
    if (password_attempt_callback_) {
      password_attempt_callback_(true, "");
    }
    
    return true;
  }
  
  stats_.password_failures++;
  hh::get_logger().warn("ServerPasswordAuth: Password verification failed");
  
  if (password_attempt_callback_) {
    password_attempt_callback_(false, "");
  }
  
  return false;
}

// ── Konfiguration ───────────────────────────────────────────────────

void ServerPasswordAuth::set_enabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  enabled_ = enabled;
}

// ── Passwort-Hinting ────────────────────────────────────────────────

void ServerPasswordAuth::set_hint(const std::string& hint) {
  std::lock_guard<std::mutex> lock(mutex_);
  hint_ = hint;
}

// ── Statistik ───────────────────────────────────────────────────────

ServerPasswordAuth::Stats ServerPasswordAuth::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void ServerPasswordAuth::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
}

// ── Callbacks ───────────────────────────────────────────────────────

void ServerPasswordAuth::set_password_attempt_callback(PasswordAttemptCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  password_attempt_callback_ = std::move(callback);
}

} // namespace hh::server

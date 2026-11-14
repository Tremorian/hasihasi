// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include "hh/client/session.hpp"

namespace hh::client {

// Reconnect-Manager: Auto-Reconnect bei Verbindungsabbruch
class ReconnectManager {
public:
  using ReconnectCallback = std::function<void(bool success, const std::string& message)>;

  struct Config {
    int max_retries = 5;
    int initial_delay_ms = 1000;
    int max_delay_ms = 16000;
    bool exponential_backoff = true;
    bool auto_reconnect = true;
  };

  explicit ReconnectManager(const Config& config = Config{});
  ~ReconnectManager() = default;

  // Nicht kopierbar, bewegbar
  ReconnectManager(const ReconnectManager&) = delete;
  ReconnectManager& operator=(const ReconnectManager&) = delete;
  ReconnectManager(ReconnectManager&&) noexcept;
  ReconnectManager& operator=(ReconnectManager&&) noexcept;

  // Reconnect starten/stoppen
  void start(std::shared_ptr<Session> session, const std::string& host, uint16_t port);
  void stop();

  // Status
  bool is_reconnecting() const { return reconnecting_; }
  int current_retry() const { return current_retry_; }
  int max_retries() const { return config_.max_retries; }
  int64_t elapsed_ms() const { return elapsed_ms_; }

  // Callback
  void set_reconnect_callback(ReconnectCallback cb) { reconnect_callback_ = std::move(cb); }

private:
  // Reconnect-Versuch starten
  void attempt_reconnect();

  // Verbindungsergebnis verarbeiten
  void on_connect_result(bool success, const std::string& message);

  // Verzögerung berechnen (exponential backoff)
  int calculate_delay_ms() const;

  // Konfiguration
  Config config_;

  // Session
  std::shared_ptr<Session> session_;
  std::string host_;
  uint16_t port_ = 0;

  // Reconnect-Zustand
  std::atomic<bool> reconnecting_{false};
  std::atomic<int> current_retry_{0};
  std::atomic<int64_t> elapsed_ms_{0};

  // Timer
  std::thread reconnect_thread_;
  std::chrono::steady_clock::time_point start_time_;

  // Callback
  ReconnectCallback reconnect_callback_;
};

} // namespace hh::client

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/reconnect_manager.hpp"

#include "hh/logging.hpp"

#include <algorithm>

namespace hh::client {

ReconnectManager::ReconnectManager(const Config& config)
  : config_(config) {
  hh::get_logger().info("ReconnectManager initialized: max_retries={}, initial_delay={}ms",
                        config.max_retries, config.initial_delay_ms);
}

ReconnectManager::ReconnectManager(ReconnectManager&&) noexcept = default;
ReconnectManager& ReconnectManager::operator=(ReconnectManager&&) noexcept = default;

void ReconnectManager::start(std::shared_ptr<Session> session, const std::string& host, uint16_t port) {
  if (reconnecting_) {
    hh::get_logger().warn("ReconnectManager already reconnecting");
    return;
  }

  session_ = session;
  host_ = host;
  port_ = port;
  reconnecting_ = true;
  current_retry_ = 0;
  elapsed_ms_ = 0;
  start_time_ = std::chrono::steady_clock::now();

  hh::get_logger().info("ReconnectManager started: {}:{} ", host, port);

  // Reconnect-Thread starten
  reconnect_thread_ = std::thread([this]() {
    attempt_reconnect();
  });
}

void ReconnectManager::stop() {
  reconnecting_ = false;
  if (reconnect_thread_.joinable()) {
    reconnect_thread_.join();
  }
  hh::get_logger().info("ReconnectManager stopped");
}

void ReconnectManager::attempt_reconnect() {
  while (reconnecting_ && current_retry_ < config_.max_retries) {
    int delay_ms = calculate_delay_ms();

    hh::get_logger().info("ReconnectManager: Attempt {}/{} in {}ms",
                          current_retry_ + 1, config_.max_retries, delay_ms);

    // Verzögerung warten (unterbrechbar)
    for (int i = 0; i < delay_ms && reconnecting_; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      elapsed_ms_++;
    }

    if (!reconnecting_) break;

    // Verbindung versuchen
    current_retry_++;

    // Session.connect() aufrufen
    bool success = session_->connect(host_, port_);

    // Ergebnis verarbeiten
    on_connect_result(success, success ? "Verbunden" : "Verbindung fehlgeschlagen");

    // Erfolg? Dann abbrechen
    if (success) {
      hh::get_logger().info("ReconnectManager: Successfully reconnected after {} attempts",
                            current_retry_);
      break;
    }
  }

  if (!reconnecting_) {
    hh::get_logger().info("ReconnectManager: Stopped by user");
  } else {
    hh::get_logger().error("ReconnectManager: Max retries ({}) reached", config_.max_retries);
    on_connect_result(false, "Maximale Wiederholungsanzahl erreicht");
  }

  reconnecting_ = false;
}

void ReconnectManager::on_connect_result(bool success, const std::string& message) {
  if (reconnect_callback_) {
    reconnect_callback_(success, message);
  }
}

int ReconnectManager::calculate_delay_ms() const {
  if (!config_.exponential_backoff) {
    return config_.initial_delay_ms;
  }

  // Exponential Backoff: initial_delay * 2^retry
  int delay = config_.initial_delay_ms;
  for (int i = 0; i < current_retry_; ++i) {
    delay *= 2;
  }

  // Maximal begrenzen
  return std::min(delay, config_.max_delay_ms);
}

} // namespace hh::client

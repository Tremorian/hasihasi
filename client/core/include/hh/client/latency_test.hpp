// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <chrono>

#include "hh/client/session.hpp"

namespace hh::client {

// Latenz-Test-Utility: Misst Round-Trip-Latenz über das Netzwerk
class LatencyTest {
public:
  using ResultCallback = std::function<void(double latency_ms, double jitter_ms)>;

  struct Config {
    int num_packets = 10;
    int packet_size = 64;
    int interval_ms = 100;
    int timeout_ms = 5000;
  };

  explicit LatencyTest(const Config& config = Config{});
  ~LatencyTest() = default;

  // Nicht kopierbar, bewegbar
  LatencyTest(const LatencyTest&) = delete;
  LatencyTest& operator=(const LatencyTest&) = delete;
  LatencyTest(LatencyTest&&) noexcept;
  LatencyTest& operator=(LatencyTest&&) noexcept;

  // Test starten
  void start(std::shared_ptr<Session> session, ResultCallback callback);
  void stop();

  // Status
  bool is_running() const { return running_; }
  double avg_latency() const { return avg_latency_; }
  double min_latency() const { return min_latency_; }
  double max_latency() const { return max_latency_; }
  double jitter() const { return jitter_; }
  int completed() const { return completed_; }
  int total() const { return total_; }

private:
  // Echo-Nachricht senden
  void send_echo(uint32_t sequence, std::shared_ptr<Session> session);

  // Echo-Ergebnis verarbeiten
  void handle_echo_result(uint32_t sequence, double latency_ms);

  // Konfiguration
  Config config_;

  // Test-Zustand
  std::atomic<bool> running_{false};
  std::atomic<int> completed_{0};
  int total_ = 0;

  // Statistik
  double avg_latency_ = 0.0;
  double min_latency_ = 1000.0;
  double max_latency_ = 0.0;
  double jitter_ = 0.0;

  // Callback
  ResultCallback result_callback_;

  // Timer
  std::chrono::steady_clock::time_point start_time_;
  std::vector<double> latency_samples_;
};

} // namespace hh::client

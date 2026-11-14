// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/latency_test.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <cmath>

namespace hh::client {

LatencyTest::LatencyTest(const Config& config)
  : config_(config)
  , total_(config.num_packets) {
  hh::get_logger().info("LatencyTest initialized: {} packets, {}ms interval",
                        config.num_packets, config.interval_ms);
}

LatencyTest::LatencyTest(LatencyTest&&) noexcept = default;
LatencyTest& LatencyTest::operator=(LatencyTest&&) noexcept = default;

void LatencyTest::start(std::shared_ptr<Session> session, ResultCallback callback) {
  if (running_) {
    hh::get_logger().warn("LatencyTest already running");
    return;
  }

  running_ = true;
  completed_ = 0;
  latency_samples_.clear();
  avg_latency_ = 0.0;
  min_latency_ = 1000.0;
  max_latency_ = 0.0;
  jitter_ = 0.0;

  result_callback_ = std::move(callback);
  start_time_ = std::chrono::steady_clock::now();

  hh::get_logger().info("LatencyTest started: {} packets", total_);

  // Echo-Nachrichten senden
  for (int i = 0; i < total_; ++i) {
    send_echo(static_cast<uint32_t>(i), session);

    // Intervall warten
    if (i < total_ - 1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(config_.interval_ms));
    }
  }

  hh::get_logger().info("LatencyTest: All packets sent, waiting for results...");
}

void LatencyTest::stop() {
  running_ = false;
  hh::get_logger().info("LatencyTest stopped");
}

void LatencyTest::send_echo(uint32_t sequence, std::shared_ptr<Session> session) {
  if (!session || !session->is_connected()) {
    hh::get_logger().warn("Cannot send echo: session not connected");
    return;
  }

  // Echo-Nachricht senden: [u32 sequence][u32 timestamp_ns]
  std::vector<uint8_t> payload(8);

  // Sequence
  payload[0] = (sequence >> 24) & 0xFF;
  payload[1] = (sequence >> 16) & 0xFF;
  payload[2] = (sequence >> 8) & 0xFF;
  payload[3] = sequence & 0xFF;

  // Timestamp (Nanosekunden)
  auto now = std::chrono::steady_clock::now();
  auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
    now.time_since_epoch()).count();

  payload[4] = (timestamp_ns >> 56) & 0xFF;
  payload[5] = (timestamp_ns >> 48) & 0xFF;
  payload[6] = (timestamp_ns >> 40) & 0xFF;
  payload[7] = (timestamp_ns >> 32) & 0xFF;

  // Senden als MSG_ECHO (Typ 8)
  session->send_voice_data(payload); // Temporär über Voice-Channel
  (void)payload;
}

void LatencyTest::handle_echo_result(uint32_t sequence, double latency_ms) {
  (void)sequence;

  latency_samples_.push_back(latency_ms);
  completed_++;

  // Statistik aktualisieren
  double sum = 0.0;
  min_latency_ = 1000.0;
  max_latency_ = 0.0;

  for (double sample : latency_samples_) {
    sum += sample;
    if (sample < min_latency_) min_latency_ = sample;
    if (sample > max_latency_) max_latency_ = sample;
  }

  avg_latency_ = sum / latency_samples_.size();

  // Jitter berechnen (Standardabweichung)
  double variance = 0.0;
  for (double sample : latency_samples_) {
    variance += (sample - avg_latency_) * (sample - avg_latency_);
  }
  jitter_ = std::sqrt(variance / latency_samples_.size());

  hh::get_logger().info("LatencyTest: Packet {} received, latency={:.1f}ms, jitter={:.1f}ms",
                        sequence, latency_ms, jitter_);

  // Callback aufrufen
  if (result_callback_) {
    result_callback_(avg_latency_, jitter_);
  }

  // Test abgeschlossen?
  if (completed_ >= total_) {
    running_ = false;
    hh::get_logger().info("LatencyTest completed: avg={:.1f}ms, min={:.1f}ms, max={:.1f}ms, jitter={:.1f}ms",
                          avg_latency_, min_latency_, max_latency_, jitter_);
  }
}

} // namespace hh::client

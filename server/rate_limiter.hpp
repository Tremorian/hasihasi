// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <deque>
#include <mutex>
#include <chrono>

namespace hh::server {

// Rate-Limiting und Bandbreitenlimit pro Nutzer
class RateLimiter {
public:
  // Konfiguration pro Nutzer
  struct UserConfig {
    uint64_t max_bandwidth_bps = 64000; // 64 kbit/s Default
    uint64_t max_packets_per_second = 100; // 100 Pakete/s
    uint64_t burst_size = 10; // Burst-Größe
    bool enabled = true;
  };

  // Server-Weite Konfiguration
  struct ServerConfig {
    uint64_t default_bandwidth_bps = 64000; // 64 kbit/s Default
    uint64_t default_packets_per_second = 100; // 100 Pakete/s
    uint64_t max_users = 500; // Maximale Nutzerzahl
    uint64_t flood_threshold = 1000; // Pakete/s für Flood-Erkennung
    uint64_t flood_ban_duration_ms = 300000; // 5 Minuten Bann bei Flood
    bool enabled = true;
  };

  RateLimiter() = default;
  ~RateLimiter() = default;

  // Initialisierung
  bool initialize(const ServerConfig& config);
  void shutdown();

  // Nutzer-Konfiguration setzen
  void set_user_config(uint32_t user_id, const UserConfig& config);
  UserConfig get_user_config(uint32_t user_id) const;

  // Bandbreite prüfen (return true if allowed)
  bool check_bandwidth(uint32_t user_id, size_t bytes);

  // Paket-Rate prüfen (return true if allowed)
  bool check_packet_rate(uint32_t user_id);

  // Flood-Erkennung
  bool is_flooding(uint32_t user_id);
  void reset_flood_counter(uint32_t user_id);

  // Bann-Verwaltung
  bool ban_for_flood(uint32_t user_id);
  bool is_banned(uint32_t user_id) const;
  void unban_user(uint32_t user_id);

  // Statistik
  struct Stats {
    uint64_t packets_allowed;
    uint64_t packets_dropped;
    uint64_t bandwidth_dropped;
    uint64_t floods_detected;
    uint64_t bans_issued;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using FloodCallback = std::function<void(uint32_t user_id, const std::string& username)>;
  void set_flood_callback(FloodCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  ServerConfig config_;
  FloodCallback flood_callback_;
};

} // namespace hh::server

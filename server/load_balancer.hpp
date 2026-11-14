// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#include "server_cluster.hpp"

namespace hh::server {

// Load-Balancing für Server-Cluster
class LoadBalancer {
public:
  enum class Strategy {
    LeastLoaded,  // Wenigst belasteter Server
    RoundRobin,   // Round-Robin
    Random,       // Zufälliger Server
    Weighted,     // Gewichteter Server
    Count         // Anzahl der Strategien
  };

  LoadBalancer() = default;
  ~LoadBalancer() = default;

  // Nicht kopierbar, bewegbar
  LoadBalancer(const LoadBalancer&) = delete;
  LoadBalancer& operator=(const LoadBalancer&) = delete;
  LoadBalancer(LoadBalancer&&) noexcept;
  LoadBalancer& operator=(LoadBalancer&&) noexcept;

  // Load-Balancing konfigurieren
  void set_strategy(Strategy strategy);
  Strategy get_strategy() const { return strategy_; }

  // Server auswählen
  ServerCluster::ServerInfo* select_server(ServerCluster& cluster);
  ServerCluster::ServerInfo* select_healthy_server(ServerCluster& cluster);

  // Gewichte für weighted strategy
  void set_server_weight(const std::string& host, int port, double weight);
  double get_server_weight(const std::string& host, int port) const;

  // Round-Robin
  void reset_round_robin();
  uint32_t get_round_robin_index() const { return round_robin_index_; }

  // Statistiken
  struct LoadBalancerStats {
    uint32_t total_requests = 0;
    uint32_t requests_per_server[10] = {}; // Max 10 Server
    double average_load = 0.0;
    double max_load = 0.0;
    double min_load = 1.0;
  };

  LoadBalancerStats get_stats() const;
  void reset_stats();

private:
  // Wenigst belasteten Server auswählen
  ServerCluster::ServerInfo* select_least_loaded(ServerCluster& cluster);
  
  // Round-Robin Server auswählen
  ServerCluster::ServerInfo* select_round_robin(ServerCluster& cluster);
  
  // Zufälligen Server auswählen
  ServerCluster::ServerInfo* select_random(ServerCluster& cluster);
  
  // Gewichteten Server auswählen
  ServerCluster::ServerInfo* select_weighted(ServerCluster& cluster);
  
  // Einstellungen
  Strategy strategy_ = Strategy::LeastLoaded;
  uint32_t round_robin_index_ = 0;
  
  // Gewichte
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::unordered_map<int, double>> weights_;
  
  // Statistiken
  mutable std::atomic<uint32_t> total_requests_{0};
  std::unordered_map<std::string, uint32_t> requests_per_server_;
};

} // namespace hh::server

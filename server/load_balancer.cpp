// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "load_balancer.hpp"

#include <algorithm>
#include <random>
#include <numeric>

namespace hh::server {

// ── LoadBalancer ──────────────────────────────────────────────────────

LoadBalancer::LoadBalancer(LoadBalancer&& other) noexcept
  : strategy_(other.strategy_)
  , round_robin_index_(other.round_robin_index_)
  , weights_(std::move(other.weights_))
  , requests_per_server_(std::move(other.requests_per_server_))
  , total_requests_(other.total_requests_.load()) {
  
  other.strategy_ = Strategy::LeastLoaded;
  other.round_robin_index_ = 0;
  other.total_requests_ = 0;
}

LoadBalancer& LoadBalancer::operator=(LoadBalancer&& other) noexcept {
  if (this != &other) {
    strategy_ = other.strategy_;
    round_robin_index_ = other.round_robin_index_;
    weights_ = std::move(other.weights_);
    requests_per_server_ = std::move(other.requests_per_server_);
    total_requests_ = other.total_requests_.load();
    other.strategy_ = Strategy::LeastLoaded;
    other.round_robin_index_ = 0;
    other.total_requests_ = 0;
  }
  return *this;
}

void LoadBalancer::set_strategy(Strategy strategy) {
  strategy_ = strategy;
}

ServerCluster::ServerInfo* LoadBalancer::select_server(ServerCluster& cluster) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  total_requests_++;
  
  switch (strategy_) {
    case Strategy::LeastLoaded: return select_least_loaded(cluster);
    case Strategy::RoundRobin: return select_round_robin(cluster);
    case Strategy::Random: return select_random(cluster);
    case Strategy::Weighted: return select_weighted(cluster);
    default: return select_least_loaded(cluster);
  }
}

ServerCluster::ServerInfo* LoadBalancer::select_healthy_server(ServerCluster& cluster) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto servers = cluster.get_all_servers();
  auto it = std::find_if(servers.begin(), servers.end(),
                         [](const ServerCluster::ServerInfo& server) { return server.is_healthy; });
  if (it == servers.end()) return nullptr;
  
  total_requests_++;
  requests_per_server_[it->host + ":" + std::to_string(it->port)]++;
  
  return &(*it);
}

void LoadBalancer::set_server_weight(const std::string& host, int port, double weight) {
  std::lock_guard<std::mutex> lock(mutex_);
  weights_[host][port] = weight;
}

double LoadBalancer::get_server_weight(const std::string& host, int port) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto host_it = weights_.find(host);
  if (host_it == weights_.end()) return 1.0;
  
  auto port_it = host_it->second.find(port);
  if (port_it == host_it->second.end()) return 1.0;
  
  return port_it->second;
}

void LoadBalancer::reset_round_robin() {
  std::lock_guard<std::mutex> lock(mutex_);
  round_robin_index_ = 0;
}

LoadBalancer::LoadBalancerStats LoadBalancer::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  LoadBalancerStats stats;
  stats.total_requests = total_requests_.load();
  
  uint32_t idx = 0;
  for (const auto& [key, count] : requests_per_server_) {
    if (idx < 10) {
      stats.requests_per_server[idx] = count;
    }
    idx++;
  }
  
  // Average/Max/Min Load wird vom Cluster berechnet
  stats.average_load = 0.0;
  stats.max_load = 0.0;
  stats.min_load = 1.0;
  
  return stats;
}

void LoadBalancer::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  total_requests_ = 0;
  requests_per_server_.clear();
}

ServerCluster::ServerInfo* LoadBalancer::select_least_loaded(ServerCluster& cluster) {
  auto servers = cluster.get_all_servers();
  auto it = std::min_element(servers.begin(), servers.end(),
                             [](const ServerCluster::ServerInfo& a, const ServerCluster::ServerInfo& b) {
                               return a.load < b.load;
                             });
  if (it == servers.end()) return nullptr;
  
  requests_per_server_[it->host + ":" + std::to_string(it->port)]++;
  return &(*it);
}

ServerCluster::ServerInfo* LoadBalancer::select_round_robin(ServerCluster& cluster) {
  auto servers = cluster.get_all_servers();
  if (servers.empty()) return nullptr;
  
  round_robin_index_ %= servers.size();
  ServerCluster::ServerInfo* selected = &servers[round_robin_index_];
  round_robin_index_++;
  
  requests_per_server_[selected->host + ":" + std::to_string(selected->port)]++;
  return selected;
}

ServerCluster::ServerInfo* LoadBalancer::select_random(ServerCluster& cluster) {
  auto servers = cluster.get_all_servers();
  if (servers.empty()) return nullptr;
  
  std::random_device rd;
  std::mt19937 g(rd());
  std::uniform_int_distribution<size_t> dist(0, servers.size() - 1);
  
  ServerCluster::ServerInfo* selected = &servers[dist(g)];
  requests_per_server_[selected->host + ":" + std::to_string(selected->port)]++;
  return selected;
}

ServerCluster::ServerInfo* LoadBalancer::select_weighted(ServerCluster& cluster) {
  auto servers = cluster.get_all_servers();
  if (servers.empty()) return nullptr;
  
  // Gesamtgewicht berechnen
  double total_weight = 0.0;
  for (const auto& server : servers) {
    total_weight += get_server_weight(server.host, server.port);
  }
  
  // Zufälligen Server basierend auf Gewicht auswählen
  std::random_device rd;
  std::mt19937 g(rd());
  std::uniform_real_distribution<double> dist(0.0, total_weight);
  
  double random_weight = dist(g);
  double cumulative_weight = 0.0;
  
  for (auto& server : servers) {
    cumulative_weight += get_server_weight(server.host, server.port);
    if (random_weight <= cumulative_weight) {
      requests_per_server_[server.host + ":" + std::to_string(server.port)]++;
      return &server;
    }
  }
  
  // Fallback zum letzten Server
  requests_per_server_[servers.back().host + ":" + std::to_string(servers.back().port)]++;
  return &servers.back();
}

} // namespace hh::server

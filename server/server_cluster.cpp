// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server_cluster.hpp"

#include <algorithm>
#include <numeric>

namespace hh::server {

// ── ServerCluster ─────────────────────────────────────────────────────

ServerCluster::ServerCluster(ServerCluster&& other) noexcept
  : servers_(std::move(other.servers_))
  , all_servers_(std::move(other.all_servers_))
  , server_health_changed_callback_(std::move(other.server_health_changed_callback_))
  , total_connected_clients_(other.total_connected_clients_.load())
  , total_max_clients_(other.total_max_clients_.load()) {
  
  other.total_connected_clients_ = 0;
  other.total_max_clients_ = 0;
}

ServerCluster& ServerCluster::operator=(ServerCluster&& other) noexcept {
  if (this != &other) {
    servers_ = std::move(other.servers_);
    all_servers_ = std::move(other.all_servers_);
    server_health_changed_callback_ = std::move(other.server_health_changed_callback_);
    total_connected_clients_ = other.total_connected_clients_.load();
    total_max_clients_ = other.total_max_clients_.load();
    other.total_connected_clients_ = 0;
    other.total_max_clients_ = 0;
  }
  return *this;
}

void ServerCluster::add_server(const std::string& host, int port, bool is_primary) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  ServerInfo server;
  server.host = host;
  server.port = port;
  server.is_primary = is_primary;
  server.is_healthy = true;
  server.connected_clients = 0;
  server.max_clients = 1000;
  server.load = 0.0;
  
  servers_[host].push_back(server);
  all_servers_.push_back(server);
  
  hh::get_logger().info("ServerCluster: Added server {}:{} (primary={})", host, port, is_primary);
}

void ServerCluster::remove_server(const std::string& host, int port) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = servers_.find(host);
  if (it == servers_.end()) return;
  
  auto& server_list = it->second;
  server_list.erase(
    std::remove_if(server_list.begin(), server_list.end(),
                   [port](const ServerInfo& server) { return server.port == port; }),
    server_list.end());
  
  all_servers_.erase(
    std::remove_if(all_servers_.begin(), all_servers_.end(),
                   [host, port](const ServerInfo& server) { return server.host == host && server.port == port; }),
    all_servers_.end());
  
  hh::get_logger().info("ServerCluster: Removed server {}:{}", host, port);
}

void ServerCluster::update_server_health(const std::string& host, int port, bool healthy) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& server : all_servers_) {
    if (server.host == host && server.port == port) {
      server.is_healthy = healthy;
      break;
    }
  }
  
  if (server_health_changed_callback_) {
    server_health_changed_callback_(host, port, healthy);
  }
  
  hh::get_logger().info("ServerCluster: Updated server {}:{} health to {}", host, port, healthy ? "healthy" : "unhealthy");
}

void ServerCluster::update_server_load(const std::string& host, int port, double load) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& server : all_servers_) {
    if (server.host == host && server.port == port) {
      server.load = load;
      break;
    }
  }
}

ServerCluster::ServerInfo* ServerCluster::get_server(const std::string& host, int port) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& server : all_servers_) {
    if (server.host == host && server.port == port) {
      return &server;
    }
  }
  return nullptr;
}

const ServerCluster::ServerInfo* ServerCluster::get_server(const std::string& host, int port) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& server : all_servers_) {
    if (server.host == host && server.port == port) {
      return &server;
    }
  }
  return nullptr;
}

std::vector<ServerCluster::ServerInfo> ServerCluster::get_all_servers() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return all_servers_;
}

ServerCluster::ServerInfo* ServerCluster::get_primary_server() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& server : all_servers_) {
    if (server.is_primary) {
      return &server;
    }
  }
  return nullptr;
}

const ServerCluster::ServerInfo* ServerCluster::get_primary_server() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& server : all_servers_) {
    if (server.is_primary) {
      return &server;
    }
  }
  return nullptr;
}

ServerCluster::ServerInfo* ServerCluster::get_least_loaded_server() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  ServerInfo* least_loaded = nullptr;
  double min_load = 1.0;
  
  for (auto& server : all_servers_) {
    if (server.is_healthy && server.load < min_load) {
      least_loaded = &server;
      min_load = server.load;
    }
  }
  
  return least_loaded;
}

ServerCluster::ServerInfo* ServerCluster::get_healthy_server() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& server : all_servers_) {
    if (server.is_healthy) {
      return &server;
    }
  }
  return nullptr;
}

bool ServerCluster::is_cluster_healthy() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& server : all_servers_) {
    if (!server.is_healthy) {
      return false;
    }
  }
  return true;
}

uint32_t ServerCluster::get_total_connected_clients() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  uint32_t total = 0;
  for (const auto& server : all_servers_) {
    total += server.connected_clients;
  }
  return total;
}

uint32_t ServerCluster::get_total_max_clients() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  uint32_t total = 0;
  for (const auto& server : all_servers_) {
    total += server.max_clients;
  }
  return total;
}

double ServerCluster::get_average_load() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (all_servers_.empty()) return 0.0;
  
  double total_load = 0.0;
  for (const auto& server : all_servers_) {
    total_load += server.load;
  }
  return total_load / all_servers_.size();
}

void ServerCluster::set_server_health_changed_callback(ServerHealthChangedCallback callback) {
  server_health_changed_callback_ = std::move(callback);
}

ServerCluster::ClusterStats ServerCluster::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  ClusterStats stats;
  stats.total_servers = all_servers_.size();
  stats.healthy_servers = std::count_if(all_servers_.begin(), all_servers_.end(),
                                        [](const ServerInfo& server) { return server.is_healthy; });
  stats.unhealthy_servers = stats.total_servers - stats.healthy_servers;
  stats.total_connected_clients = std::accumulate(all_servers_.begin(), all_servers_.end(), 0U,
                                                  [](uint32_t sum, const ServerInfo& server) { return sum + server.connected_clients; });
  stats.total_max_clients = std::accumulate(all_servers_.begin(), all_servers_.end(), 0U,
                                            [](uint32_t sum, const ServerInfo& server) { return sum + server.max_clients; });
  stats.average_load = get_average_load();
  
  return stats;
}

} // namespace hh::server

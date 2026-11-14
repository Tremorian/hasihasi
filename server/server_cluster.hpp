// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

namespace hh::server {

// Server-Cluster für Multi-Server-Betrieb
class ServerCluster {
public:
  struct ServerInfo {
    std::string host;
    int port;
    bool is_primary = false;
    bool is_healthy = false;
    uint32_t connected_clients = 0;
    uint32_t max_clients = 0;
    double load = 0.0;
  };

  ServerCluster() = default;
  ~ServerCluster() = default;

  // Nicht kopierbar, bewegbar
  ServerCluster(const ServerCluster&) = delete;
  ServerCluster& operator=(const ServerCluster&) = delete;
  ServerCluster(ServerCluster&&) noexcept;
  ServerCluster& operator=(ServerCluster&&) noexcept;

  // Server verwalten
  void add_server(const std::string& host, int port, bool is_primary = false);
  void remove_server(const std::string& host, int port);
  void update_server_health(const std::string& host, int port, bool healthy);
  void update_server_load(const std::string& host, int port, double load);

  // Server abrufen
  ServerInfo* get_server(const std::string& host, int port);
  const ServerInfo* get_server(const std::string& host, int port) const;
  std::vector<ServerInfo> get_all_servers() const;
  ServerInfo* get_primary_server();
  const ServerInfo* get_primary_server() const;

  // Load-Balancing
  ServerInfo* get_least_loaded_server();
  ServerInfo* get_healthy_server();

  // Cluster-Status
  bool is_cluster_healthy() const;
  uint32_t get_total_connected_clients() const;
  uint32_t get_total_max_clients() const;
  double get_average_load() const;

  // Callbacks
  using ServerHealthChangedCallback = std::function<void(const std::string& host, int port, bool healthy)>;
  void set_server_health_changed_callback(ServerHealthChangedCallback callback);

  // Statistiken
  struct ClusterStats {
    uint32_t total_servers = 0;
    uint32_t healthy_servers = 0;
    uint32_t unhealthy_servers = 0;
    uint32_t total_connected_clients = 0;
    uint32_t total_max_clients = 0;
    double average_load = 0.0;
  };

  ClusterStats get_stats() const;

private:
  // Server-Liste aktualisieren
  void update_server_list();

  // Server-Health prüfen
  bool is_server_healthy(const ServerInfo& server) const;

  // Einstellungen
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::vector<ServerInfo>> servers_; // host -> servers
  std::vector<ServerInfo> all_servers_;
  
  // Callbacks
  ServerHealthChangedCallback server_health_changed_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> total_connected_clients_{0};
  mutable std::atomic<uint32_t> total_max_clients_{0};
};

} // namespace hh::server

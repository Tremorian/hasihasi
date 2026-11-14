// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>

namespace hh::server {

// Server-Discovery über mDNS/Bonjour
// Findet HasiHasi-Server im lokalen Netzwerk
class ServerDiscovery {
public:
  struct DiscoveredServer {
    std::string hostname;
    std::string host;
    int port;
    std::string service_name;
    double signal_strength = 0.0;
    uint32_t connected_clients = 0;
    uint32_t max_clients = 0;
  };

  ServerDiscovery() = default;
  ~ServerDiscovery() = default;

  // Nicht kopierbar, bewegbar
  ServerDiscovery(const ServerDiscovery&) = delete;
  ServerDiscovery& operator=(const ServerDiscovery&) = delete;
  ServerDiscovery(ServerDiscovery&&) noexcept;
  ServerDiscovery& operator=(ServerDiscovery&&) noexcept;

  // Discovery starten/stoppen
  void start_discovery();
  void stop_discovery();
  bool is_discovery_active() const { return is_active_; }

  // Gefundene Server abrufen
  std::vector<DiscoveredServer> get_discovered_servers() const;
  DiscoveredServer* find_server_by_hostname(const std::string& hostname);
  const DiscoveredServer* find_server_by_hostname(const std::string& hostname) const;

  // Callbacks
  using ServerFoundCallback = std::function<void(const DiscoveredServer& server)>;
  using ServerLostCallback = std::function<void(const std::string& hostname)>;
  
  void set_server_found_callback(ServerFoundCallback callback);
  void set_server_lost_callback(ServerLostCallback callback);

  // Discovery-Einstellungen
  void set_discovery_interval_ms(int interval_ms);
  int get_discovery_interval_ms() const { return discovery_interval_ms_; }

  void set_service_name(const std::string& service_name);
  std::string get_service_name() const { return service_name_; }

private:
  // Discovery-Schritt ausführen
  void discover_servers();
  
  // Server-Timeout prüfen
  void check_server_timeouts();
  
  // Einstellungen
  bool is_active_ = false;
  int discovery_interval_ms_ = 5000; // 5 Sekunden
  std::string service_name_ = "_hasihasi._tcp.local.";
  
  // Gefundene Server
  mutable std::mutex mutex_;
  std::vector<DiscoveredServer> discovered_servers_;
  
  // Callbacks
  ServerFoundCallback server_found_callback_;
  ServerLostCallback server_lost_callback_;
  
  // Timer (simuliert)
  std::unique_ptr<std::thread> discovery_thread_;
};

} // namespace hh::server

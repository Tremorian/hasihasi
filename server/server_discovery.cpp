// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server_discovery.hpp"

#include "hh/logging.hpp"

#include <thread>
#include <chrono>
#include <algorithm>

namespace hh::server {

// ── ServerDiscovery ───────────────────────────────────────────────────

ServerDiscovery::ServerDiscovery(ServerDiscovery&& other) noexcept
  : is_active_(other.is_active_)
  , discovery_interval_ms_(other.discovery_interval_ms_)
  , service_name_(std::move(other.service_name_))
  , discovered_servers_(std::move(other.discovered_servers_))
  , server_found_callback_(std::move(other.server_found_callback_))
  , server_lost_callback_(std::move(other.server_lost_callback_))
  , discovery_thread_(std::move(other.discovery_thread_)) {
  
  other.is_active_ = false;
  other.discovery_interval_ms_ = 5000;
}

ServerDiscovery& ServerDiscovery::operator=(ServerDiscovery&& other) noexcept {
  if (this != &other) {
    is_active_ = other.is_active_;
    discovery_interval_ms_ = other.discovery_interval_ms_;
    service_name_ = std::move(other.service_name_);
    discovered_servers_ = std::move(other.discovered_servers_);
    server_found_callback_ = std::move(other.server_found_callback_);
    server_lost_callback_ = std::move(other.server_lost_callback_);
    discovery_thread_ = std::move(other.discovery_thread_);
    other.is_active_ = false;
    other.discovery_interval_ms_ = 5000;
  }
  return *this;
}

void ServerDiscovery::start_discovery() {
  if (is_active_) {
    hh::get_logger().warn("ServerDiscovery: Discovery already active");
    return;
  }
  
  is_active_ = true;
  
  // Discovery-Thread starten
  discovery_thread_ = std::make_unique<std::thread>([this]() {
    while (is_active_) {
      discover_servers();
      check_server_timeouts();
      std::this_thread::sleep_for(std::chrono::milliseconds(discovery_interval_ms_));
    }
  });
  
  hh::get_logger().info("ServerDiscovery: Discovery started");
}

void ServerDiscovery::stop_discovery() {
  if (!is_active_) {
    hh::get_logger().warn("ServerDiscovery: Discovery not active");
    return;
  }
  
  is_active_ = false;
  
  if (discovery_thread_ && discovery_thread_->joinable()) {
    discovery_thread_->join();
  }
  
  hh::get_logger().info("ServerDiscovery: Discovery stopped");
}

std::vector<ServerDiscovery::DiscoveredServer> ServerDiscovery::get_discovered_servers() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return discovered_servers_;
}

ServerDiscovery::DiscoveredServer* ServerDiscovery::find_server_by_hostname(const std::string& hostname) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = std::find_if(discovered_servers_.begin(), discovered_servers_.end(),
                         [&hostname](const DiscoveredServer& server) { return server.hostname == hostname; });
  if (it != discovered_servers_.end()) {
    return &(*it);
  }
  return nullptr;
}

const ServerDiscovery::DiscoveredServer* ServerDiscovery::find_server_by_hostname(const std::string& hostname) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = std::find_if(discovered_servers_.begin(), discovered_servers_.end(),
                         [&hostname](const DiscoveredServer& server) { return server.hostname == hostname; });
  if (it != discovered_servers_.end()) {
    return &(*it);
  }
  return nullptr;
}

void ServerDiscovery::set_server_found_callback(ServerFoundCallback callback) {
  server_found_callback_ = std::move(callback);
}

void ServerDiscovery::set_server_lost_callback(ServerLostCallback callback) {
  server_lost_callback_ = std::move(callback);
}

void ServerDiscovery::set_discovery_interval_ms(int interval_ms) {
  discovery_interval_ms_ = interval_ms;
}

void ServerDiscovery::set_service_name(const std::string& service_name) {
  service_name_ = service_name;
}

void ServerDiscovery::discover_servers() {
  // mDNS/Bonjour-Discovery simuliert
  // In einer echten Implementierung würde hier avahi-browse oder similar verwendet
  
  hh::get_logger().debug("ServerDiscovery: Discovering servers...");
  
  // Simulierte Server-Discovery
  // In der Praxis würde hier der mDNS-Browser ausgeführt werden
}

void ServerDiscovery::check_server_timeouts() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Server-Timeout prüfen (simuliert)
  // In einer echten Implementierung würde hier die Last-Updated-Zeit geprüft werden
}

} // namespace hh::server

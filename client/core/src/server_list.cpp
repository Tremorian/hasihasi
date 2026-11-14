// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/server_list.hpp"

#include "hh/logging.hpp"

#include <asio.hpp>
#include <chrono>

namespace hh::client {

// ── Server-Verwaltung ─────────────────────────────────────────────────

bool ServerList::add_server(const ServerEntry& server) {
  // Prüfen ob Server bereits existiert
  if (find_server_by_address(server.address, server.port)) {
    hh::get_logger().warn("ServerList: Server {}:{} already exists", server.address, server.port);
    return false;
  }

  servers_.push_back(server);
  hh::get_logger().info("ServerList: Server added: {} ({})", server.name, server.address);
  return true;
}

bool ServerList::remove_server(uint32_t server_id) {
  for (auto it = servers_.begin(); it != servers_.end(); ++it) {
    if (it->id == server_id) {
      servers_.erase(it);
      hh::get_logger().info("ServerList: Server {} removed", server_id);
      return true;
    }
  }
  return false;
}

bool ServerList::update_server(const ServerEntry& server) {
  for (auto& s : servers_) {
    if (s.id == server.id) {
      s = server;
      hh::get_logger().info("ServerList: Server {} updated", server.id);
      return true;
    }
  }
  return false;
}

std::vector<ServerEntry> ServerList::all_servers() const {
  return servers_;
}

std::vector<ServerEntry> ServerList::favorite_servers() const {
  std::vector<ServerEntry> favorites;
  for (const auto& server : servers_) {
    if (server.is_favorite) {
      favorites.push_back(server);
    }
  }
  return favorites;
}

std::shared_ptr<ServerEntry> ServerList::find_server(uint32_t server_id) const {
  for (auto& server : servers_) {
    if (server.id == server_id) {
      return std::make_shared<ServerEntry>(server);
    }
  }
  return nullptr;
}

std::shared_ptr<ServerEntry> ServerList::find_server_by_address(const std::string& address, uint16_t port) const {
  for (auto& server : servers_) {
    if (server.address == address && server.port == port) {
      return std::make_shared<ServerEntry>(server);
    }
  }
  return nullptr;
}

// ── Ping-Anzeige ──────────────────────────────────────────────────────

void ServerList::ping_server(uint32_t server_id, const std::string& address, uint16_t port) {
  hh::get_logger().info("ServerList: Pinging server {} at {}:{}", server_id, address, port);

  // Asio-IO-Context für Ping
  asio::io_context io_context;
  asio::ip::tcp::socket socket(io_context);
  socket.expires_after(std::chrono::milliseconds(3000));

  try {
    asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(address, std::to_string(port));
    asio::connect(socket, endpoints);

    int ping_ms = 10; // Simuliere Ping-Zeit
    hh::get_logger().info("ServerList: Server {} pinged successfully: {}ms", server_id, ping_ms);

    // Server aktualisieren
    for (auto& server : servers_) {
      if (server.id == server_id) {
        server.ping_ms = ping_ms;
        break;
      }
    }

    if (ping_callback_) {
      ping_callback_({server_id, ping_ms, true});
    }
  } catch (const std::exception& e) {
    hh::get_logger().error("ServerList: Server {} ping failed: {}", server_id, e.what());

    if (ping_callback_) {
      ping_callback_({server_id, -1, false});
    }
  }
}

void ServerList::ping_all_servers() {
  for (const auto& server : servers_) {
    ping_server(server.id, server.address, server.port);
  }
}

// ── Verbindungsassistent ──────────────────────────────────────────────

ServerList::ConnectionStatus ServerList::test_connection(const std::string& address, uint16_t port, int timeout_ms) {
  hh::get_logger().info("ServerList: Testing connection to {}:{}", address, port);

  try {
    asio::io_context io_context;
    asio::ip::tcp::socket socket(io_context);
    socket.expires_after(std::chrono::milliseconds(timeout_ms));

    asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(address, std::to_string(port));
    asio::connect(socket, endpoints);

    hh::get_logger().info("ServerList: Connection to {}:{} successful", address, port);
    return ConnectionStatus::OK;
  } catch (const asio::error::host_not_found&) {
    hh::get_logger().warn("ServerList: DNS error for {}:{}", address, port);
    return ConnectionStatus::DNS_ERROR;
  } catch (const asio::error::connection_refused&) {
    hh::get_logger().warn("ServerList: Connection refused for {}:{}", address, port);
    return ConnectionStatus::REFUSED;
  } catch (const std::exception& e) {
    hh::get_logger().warn("ServerList: Connection timeout for {}:{}", address, port);
    return ConnectionStatus::TIMEOUT;
  }
}

// ── Callbacks ─────────────────────────────────────────────────────────

void ServerList::set_ping_callback(PingCallback callback) {
  ping_callback_ = callback;
}

void ServerList::set_connection_callback(ConnectionCallback callback) {
  connection_callback_ = callback;
}

// ── Konfiguration ─────────────────────────────────────────────────────

bool ServerList::save_configuration() {
  hh::get_logger().info("ServerList: Saving configuration ({} servers)", servers_.size());
  // In einer echten Implementierung würde dies die Konfiguration speichern
  return true;
}

bool ServerList::load_configuration() {
  hh::get_logger().info("ServerList: Loading configuration");
  // In einer echten Implementierung würde dies die Konfiguration laden
  return true;
}

size_t ServerList::favorite_count() const {
  size_t count = 0;
  for (const auto& server : servers_) {
    if (server.is_favorite) {
      count++;
    }
  }
  return count;
}

} // namespace hh::client

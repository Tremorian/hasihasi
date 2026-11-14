// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace hh::client {

// Server-Eintrag in der Serverliste
struct ServerEntry {
  uint32_t id;
  std::string name;
  std::string address;
  uint16_t port;
  bool is_favorite;
  int ping_ms; // -1 wenn nicht gepingt
  std::string description;
};

// Serverliste mit Favoriten, Ping-Anzeige und Verbindungsassistent
class ServerList {
public:
  ServerList() = default;
  ~ServerList() = default;

  // Server hinzufügen/entfernen
  bool add_server(const ServerEntry& server);
  bool remove_server(uint32_t server_id);
  bool update_server(const ServerEntry& server);

  // Server abrufen
  std::vector<ServerEntry> all_servers() const;
  std::vector<ServerEntry> favorite_servers() const;
  std::shared_ptr<ServerEntry> find_server(uint32_t server_id) const;
  std::shared_ptr<ServerEntry> find_server_by_address(const std::string& address, uint16_t port) const;

  // Ping-Anzeige
  struct PingResult {
    uint32_t server_id;
    int ping_ms;
    bool success;
  };

  void ping_server(uint32_t server_id, const std::string& address, uint16_t port);
  void ping_all_servers();

  // Verbindungsassistent
  enum class ConnectionStatus {
    OK,
    TIMEOUT,
    REFUSED,
    DNS_ERROR
  };

  ConnectionStatus test_connection(const std::string& address, uint16_t port, int timeout_ms = 3000);

  // Callbacks
  using PingCallback = std::function<void(const PingResult&)>;
  void set_ping_callback(PingCallback callback);

  using ConnectionCallback = std::function<void(uint32_t server_id, ConnectionStatus status)>;
  void set_connection_callback(ConnectionCallback callback);

  // Konfiguration speichern/laden
  bool save_configuration();
  bool load_configuration();

  // Statistik
  size_t server_count() const { return servers_.size(); }
  size_t favorite_count() const;

private:
  std::vector<ServerEntry> servers_;
  PingCallback ping_callback_;
  ConnectionCallback connection_callback_;
};

} // namespace hh::client

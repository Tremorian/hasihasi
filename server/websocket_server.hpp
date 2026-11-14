// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace hh::server {

// WebSocket-Server für Web-Clients
class WebSocketServer {
public:
  struct WebSocketClient {
    uint32_t client_id;
    std::string username;
    std::string host;
    int port;
    bool is_authenticated = false;
    bool is_connected = false;
    uint32_t channel_id = 0;
    double last_activity = 0.0;
  };

  WebSocketServer() = default;
  ~WebSocketServer() = default;

  // Nicht kopierbar, bewegbar
  WebSocketServer(const WebSocketServer&) = delete;
  WebSocketServer& operator=(const WebSocketServer&) = delete;
  WebSocketServer(WebSocketServer&&) noexcept;
  WebSocketServer& operator=(WebSocketServer&&) noexcept;

  // Server konfigurieren
  void set_enabled(bool enabled);
  bool is_enabled() const { return is_enabled_; }

  void set_port(int port);
  int get_port() const { return port_; }

  void set_tls_enabled(bool enabled);
  bool is_tls_enabled() const { return tls_enabled_; }

  // Server starten/stoppen
  bool start_server();
  void stop_server();
  bool is_server_running() const { return is_running_; }

  // Clients verwalten
  void add_client(const WebSocketClient& client);
  void remove_client(uint32_t client_id);
  WebSocketClient* get_client(uint32_t client_id);
  const WebSocketClient* get_client(uint32_t client_id) const;
  std::vector<WebSocketClient> get_all_clients() const;

  // Nachrichten senden
  bool send_to_client(uint32_t client_id, const std::string& message);
  bool broadcast(const std::string& message);
  bool send_to_channel(uint32_t channel_id, const std::string& message);

  // Callbacks
  using ClientConnectedCallback = std::function<void(const WebSocketClient& client)>;
  using ClientDisconnectedCallback = std::function<void(uint32_t client_id)>;
  using MessageReceivedCallback = std::function<void(uint32_t client_id, const std::string& message)>;
  
  void set_client_connected_callback(ClientConnectedCallback callback);
  void set_client_disconnected_callback(ClientDisconnectedCallback callback);
  void set_message_received_callback(MessageReceivedCallback callback);

  // Statistiken
  struct WebSocketStats {
    uint32_t total_clients = 0;
    uint32_t connected_clients = 0;
    uint32_t disconnected_clients = 0;
    uint32_t messages_sent = 0;
    uint32_t messages_received = 0;
    double total_bytes_sent = 0.0;
    double total_bytes_received = 0.0;
  };

  WebSocketStats get_stats() const;
  void reset_stats();

private:
  // Server-Thread
  void server_thread_func();
  
  // Client-Thread
  void client_thread_func(uint32_t client_id);
  
  // Einstellungen
  bool is_enabled_ = false;
  bool is_running_ = false;
  int port_ = 8080;
  bool tls_enabled_ = false;
  
  // Clients
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, WebSocketClient> clients_;
  uint32_t next_client_id_ = 1;
  
  // Callbacks
  ClientConnectedCallback client_connected_callback_;
  ClientDisconnectedCallback client_disconnected_callback_;
  MessageReceivedCallback message_received_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> messages_sent_{0};
  mutable std::atomic<uint32_t> messages_received_{0};
  mutable std::atomic<double> total_bytes_sent_{0.0};
  mutable std::atomic<double> total_bytes_received_{0.0};
  
  // Server-Thread
  std::unique_ptr<std::thread> server_thread_;
};

} // namespace hh::server

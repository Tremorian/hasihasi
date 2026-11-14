// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "websocket_server.hpp"

#include "hh/logging.hpp"

#include <thread>
#include <chrono>
#include <algorithm>

namespace hh::server {

// ── WebSocketServer ───────────────────────────────────────────────────

WebSocketServer::WebSocketServer(WebSocketServer&& other) noexcept
  : is_enabled_(other.is_enabled_)
  , is_running_(other.is_running_)
  , port_(other.port_)
  , tls_enabled_(other.tls_enabled_)
  , clients_(std::move(other.clients_))
  , next_client_id_(other.next_client_id_)
  , client_connected_callback_(std::move(other.client_connected_callback_))
  , client_disconnected_callback_(std::move(other.client_disconnected_callback_))
  , message_received_callback_(std::move(other.message_received_callback_))
  , messages_sent_(other.messages_sent_.load())
  , messages_received_(other.messages_received_.load())
  , total_bytes_sent_(other.total_bytes_sent_.load())
  , total_bytes_received_(other.total_bytes_received_.load())
  , server_thread_(std::move(other.server_thread_)) {
  
  other.is_enabled_ = false;
  other.is_running_ = false;
  other.port_ = 8080;
  other.tls_enabled_ = false;
  other.next_client_id_ = 1;
  other.messages_sent_ = 0;
  other.messages_received_ = 0;
  other.total_bytes_sent_ = 0.0;
  other.total_bytes_received_ = 0.0;
}

WebSocketServer& WebSocketServer::operator=(WebSocketServer&& other) noexcept {
  if (this != &other) {
    is_enabled_ = other.is_enabled_;
    is_running_ = other.is_running_;
    port_ = other.port_;
    tls_enabled_ = other.tls_enabled_;
    clients_ = std::move(other.clients_);
    next_client_id_ = other.next_client_id_;
    client_connected_callback_ = std::move(other.client_connected_callback_);
    client_disconnected_callback_ = std::move(other.client_disconnected_callback_);
    message_received_callback_ = std::move(other.message_received_callback_);
    messages_sent_ = other.messages_sent_.load();
    messages_received_ = other.messages_received_.load();
    total_bytes_sent_ = other.total_bytes_sent_.load();
    total_bytes_received_ = other.total_bytes_received_.load();
    server_thread_ = std::move(other.server_thread_);
    other.is_enabled_ = false;
    other.is_running_ = false;
    other.port_ = 8080;
    other.tls_enabled_ = false;
    other.next_client_id_ = 1;
    other.messages_sent_ = 0;
    other.messages_received_ = 0;
    other.total_bytes_sent_ = 0.0;
    other.total_bytes_received_ = 0.0;
  }
  return *this;
}

void WebSocketServer::set_enabled(bool enabled) {
  is_enabled_ = enabled;
  hh::get_logger().info("WebSocketServer: Enabled: {}", enabled);
}

void WebSocketServer::set_port(int port) {
  port_ = port;
}

void WebSocketServer::set_tls_enabled(bool enabled) {
  tls_enabled_ = enabled;
}

bool WebSocketServer::start_server() {
  if (is_running_) {
    hh::get_logger().warn("WebSocketServer: Server already running");
    return false;
  }
  
  if (!is_enabled_) {
    hh::get_logger().warn("WebSocketServer: Server not enabled");
    return false;
  }
  
  is_running_ = true;
  
  // Server-Thread starten
  server_thread_ = std::make_unique<std::thread>([this]() {
    server_thread_func();
  });
  
  hh::get_logger().info("WebSocketServer: Server started on port {}", port_);
  return true;
}

void WebSocketServer::stop_server() {
  if (!is_running_) {
    hh::get_logger().warn("WebSocketServer: Server not running");
    return;
  }
  
  is_running_ = false;
  
  if (server_thread_ && server_thread_->joinable()) {
    server_thread_->join();
  }
  
  hh::get_logger().info("WebSocketServer: Server stopped");
}

void WebSocketServer::add_client(const WebSocketClient& client) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  clients_[client.client_id] = client;
  
  if (client_connected_callback_) {
    client_connected_callback_(client);
  }
  
  hh::get_logger().info("WebSocketServer: Client {} connected", client.client_id);
}

void WebSocketServer::remove_client(uint32_t client_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = clients_.find(client_id);
  if (it == clients_.end()) return;
  
  clients_.erase(it);
  
  if (client_disconnected_callback_) {
    client_disconnected_callback_(client_id);
  }
  
  hh::get_logger().info("WebSocketServer: Client {} disconnected", client_id);
}

WebSocketServer::WebSocketClient* WebSocketServer::get_client(uint32_t client_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = clients_.find(client_id);
  if (it != clients_.end()) {
    return &it->second;
  }
  return nullptr;
}

const WebSocketServer::WebSocketClient* WebSocketServer::get_client(uint32_t client_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = clients_.find(client_id);
  if (it != clients_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<WebSocketServer::WebSocketClient> WebSocketServer::get_all_clients() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<WebSocketClient> clients;
  for (const auto& [id, client] : clients_) {
    clients.push_back(client);
  }
  return clients;
}

bool WebSocketServer::send_to_client(uint32_t client_id, const std::string& message) {
  // In einer echten Implementierung würde hier die Nachricht über WebSocket gesendet
  messages_sent_++;
  total_bytes_sent_ += message.size();
  return true;
}

bool WebSocketServer::broadcast(const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& [id, client] : clients_) {
    if (client.is_connected) {
      // Nachricht senden
      messages_sent_++;
      total_bytes_sent_ += message.size();
    }
  }
  
  return true;
}

bool WebSocketServer::send_to_channel(uint32_t channel_id, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& [id, client] : clients_) {
    if (client.is_connected && client.channel_id == channel_id) {
      // Nachricht senden
      messages_sent_++;
      total_bytes_sent_ += message.size();
    }
  }
  
  return true;
}

void WebSocketServer::set_client_connected_callback(ClientConnectedCallback callback) {
  client_connected_callback_ = std::move(callback);
}

void WebSocketServer::set_client_disconnected_callback(ClientDisconnectedCallback callback) {
  client_disconnected_callback_ = std::move(callback);
}

void WebSocketServer::set_message_received_callback(MessageReceivedCallback callback) {
  message_received_callback_ = std::move(callback);
}

WebSocketServer::WebSocketStats WebSocketServer::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  WebSocketStats stats;
  stats.total_clients = clients_.size();
  stats.connected_clients = std::count_if(clients_.begin(), clients_.end(),
                                          [](const auto& pair) { return pair.second.is_connected; });
  stats.disconnected_clients = stats.total_clients - stats.connected_clients;
  stats.messages_sent = messages_sent_.load();
  stats.messages_received = messages_received_.load();
  stats.total_bytes_sent = total_bytes_sent_.load();
  stats.total_bytes_received = total_bytes_received_.load();
  
  return stats;
}

void WebSocketServer::reset_stats() {
  messages_sent_ = 0;
  messages_received_ = 0;
  total_bytes_sent_ = 0.0;
  total_bytes_received_ = 0.0;
}

void WebSocketServer::server_thread_func() {
  // WebSocket-Server-Thread
  // In einer echten Implementierung würde hier der WebSocket-Server laufen
  hh::get_logger().info("WebSocketServer: Server thread started");
  
  while (is_running_) {
    // Clients prüfen
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  
  hh::get_logger().info("WebSocketServer: Server thread stopped");
}

void WebSocketServer::client_thread_func(uint32_t client_id) {
  // Client-Thread
  hh::get_logger().info("WebSocketServer: Client {} thread started", client_id);
  
  // Client-Logik
  
  hh::get_logger().info("WebSocketServer: Client {} thread stopped", client_id);
}

} // namespace hh::server

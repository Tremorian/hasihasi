// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace hh::server {

// Unix-Socket Server für Admin-CLI
class AdminServer {
public:
  using MessageHandler = std::function<void(const std::string& command,
                                             const std::map<std::string, std::string>& args,
                                             std::string& response)>;

  explicit AdminServer(asio::io_context& ioc);
  ~AdminServer();

  // Server starten
  bool start(const std::string& socket_path = "/var/run/hasihasi/hasihasi.sock");

  // Server stoppen
  void stop();

  // Handler registrieren
  void set_message_handler(MessageHandler handler);

  // Status
  bool is_running() const { return is_running_; }
  const std::string& socket_path() const { return socket_path_; }

private:
  void do_accept();
  void handle_client(std::unique_ptr<asio::stream_socket> socket);
  void read_client(std::unique_ptr<asio::stream_socket> socket, std::string buffer);

  asio::io_context& ioc_;
  std::unique_ptr<asio::stream_socket> accept_socket_;
  std::string socket_path_;
  bool is_running_ = false;
  MessageHandler message_handler_;
};

// Admin-CLI Client
class AdminClient {
public:
  AdminClient();
  ~AdminClient();

  // Verbindung zum Server
  bool connect(const std::string& socket_path = "/var/run/hasihasi/hasihasi.sock");
  void disconnect();
  bool is_connected() const { return is_connected_; }

  // Befehle senden
  bool send_command(const std::string& command, const std::map<std::string, std::string>& args = {},
                    std::string& response = "");

  // Spezifische Befehle
  bool get_users(std::vector<std::string>& users);
  bool get_channels(std::vector<std::string>& channels);
  bool get_metrics(std::string& metrics);
  bool ban_user(uint32_t user_id, const std::string& reason);
  bool kick_user(uint32_t user_id, const std::string& reason);
  bool set_user_muted(uint32_t user_id, bool muted);
  bool set_user_deafened(uint32_t user_id, bool deafened);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool is_connected_ = false;
};

} // namespace hh::server

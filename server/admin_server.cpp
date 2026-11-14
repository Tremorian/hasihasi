// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/admin_server.hpp"
#include "hh/logging.hpp"

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <sys/un.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

namespace hh::server {

// ── AdminServer ───────────────────────────────────────────────────────

AdminServer::AdminServer(asio::io_context& ioc)
  : ioc_(ioc)
  , accept_socket_(std::make_unique<asio::stream_socket>(ioc)) {
}

AdminServer::~AdminServer() {
  stop();
}

bool AdminServer::start(const std::string& socket_path) {
  socket_path_ = socket_path;

  // Alten Socket entfernen, falls vorhanden
  std::error_code ec;
  std::filesystem::remove(socket_path, ec);

  // Unix-Domain-Socket erstellen
  accept_socket_ = std::make_unique<asio::stream_socket>(ioc_, asio::stream_socket::get_protocol());
  
  // Socket binden
  asio::unix::stream_protocol::endpoint endpoint(socket_path);
  accept_socket_->bind(endpoint, ec);
  if (ec) {
    hh::get_logger().error("AdminServer: Failed to bind to {}: {}", socket_path, ec.message());
    return false;
  }

  // Socket hören
  accept_socket_->listen(5, ec);
  if (ec) {
    hh::get_logger().error("AdminServer: Failed to listen: {}", ec.message());
    return false;
  }

  // Permissions setzen (770 für hasihasi Gruppe)
  chmod(socket_path.c_str(), 0770);

  is_running_ = true;
  hh::get_logger().info("AdminServer: Started on {}", socket_path);

  // Accept-Loop starten
  do_accept();
  return true;
}

void AdminServer::stop() {
  if (!is_running_) {
    return;
  }

  is_running_ = false;
  accept_socket_.reset();

  // Socket entfernen
  std::error_code ec;
  std::filesystem::remove(socket_path_, ec);

  hh::get_logger().info("AdminServer: Stopped");
}

void AdminServer::set_message_handler(MessageHandler handler) {
  message_handler_ = std::move(handler);
}

void AdminServer::do_accept() {
  auto client_socket = std::make_unique<asio::stream_socket>(ioc_);
  
  accept_socket_->async_accept(*client_socket, [this, client_socket = std::move(client_socket)](std::error_code ec) {
    if (!ec) {
      handle_client(std::move(client_socket));
    }
    if (is_running_) {
      do_accept();
    }
  });
}

void AdminServer::handle_client(std::unique_ptr<asio::stream_socket> socket) {
  hh::get_logger().info("AdminServer: Client connected");

  // Daten lesen
  auto buffer = std::make_shared<std::vector<uint8_t>>(4096, 0);
  
  socket->async_receive(asio::buffer(*buffer), [this, socket = std::move(socket), buffer](std::error_code ec, size_t bytes_transferred) {
    if (!ec) {
      std::string request(reinterpret_cast<char*>(buffer->data()), bytes_transferred);
      read_client(std::move(socket), request);
    } else {
      hh::get_logger().warn("AdminServer: Client receive failed: {}", ec.message());
    }
  });
}

void AdminServer::read_client(std::unique_ptr<asio::stream_socket> socket, std::string buffer) {
  // Befehl parsen
  std::stringstream ss(buffer);
  std::string command;
  ss >> command;

  // Argumente parsen (key=value)
  std::map<std::string, std::string> args;
  std::string arg;
  while (std::getline(ss, arg, ' ')) {
    auto eq_pos = arg.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = arg.substr(0, eq_pos);
      std::string value = arg.substr(eq_pos + 1);
      args[key] = value;
    }
  }

  // Handler aufrufen
  std::string response;
  if (message_handler_) {
    message_handler_(command, args, response);
  } else {
    response = "ERROR: No handler registered\n";
  }

  // Antwort senden
  std::error_code ec;
  asio::write(*socket, asio::buffer(response), ec);
  if (ec) {
    hh::get_logger().warn("AdminServer: Client send failed: {}", ec.message());
  }

  // Socket schließen
  std::error_code close_ec;
  socket->close(close_ec);
  hh::get_logger().info("AdminServer: Client disconnected");
}

// ── AdminClient ───────────────────────────────────────────────────────

struct AdminClient::Impl {
  asio::io_context ioc;
  std::unique_ptr<asio::stream_socket> socket;
  std::string socket_path;
};

AdminClient::AdminClient()
  : impl_(std::make_unique<Impl>()) {
}

AdminClient::~AdminClient() {
  disconnect();
}

bool AdminClient::connect(const std::string& socket_path) {
  impl_->socket_path = socket_path;

  // Alten Socket entfernen, falls vorhanden
  std::error_code ec;
  std::filesystem::remove(socket_path, ec);

  // Socket erstellen
  impl_->socket = std::make_unique<asio::stream_socket>(impl_->ioc, asio::stream_socket::get_protocol());
  
  // Socket verbinden
  asio::unix::stream_protocol::endpoint endpoint(socket_path);
  impl_->socket->connect(endpoint, ec);
  if (ec) {
    hh::get_logger().error("AdminClient: Failed to connect to {}: {}", socket_path, ec.message());
    return false;
  }

  is_connected_ = true;
  hh::get_logger().info("AdminClient: Connected to {}", socket_path);
  return true;
}

void AdminClient::disconnect() {
  if (!is_connected_) {
    return;
  }

  if (impl_->socket) {
    std::error_code ec;
    impl_->socket->close(ec);
  }

  is_connected_ = false;
  hh::get_logger().info("AdminClient: Disconnected");
}

bool AdminClient::send_command(const std::string& command, const std::map<std::string, std::string>& args,
                                std::string& response) {
  if (!is_connected_ || !impl_->socket) {
    hh::get_logger().error("AdminClient: Not connected");
    return false;
  }

  // Befehl aufbauen
  std::stringstream ss;
  ss << command;
  for (const auto& [key, value] : args) {
    ss << " " << key << "=" << value;
  }
  ss << "\n";

  std::string request = ss.str();

  // Befehl senden
  std::error_code ec;
  asio::write(*impl_->socket, asio::buffer(request), ec);
  if (ec) {
    hh::get_logger().error("AdminClient: Send failed: {}", ec.message());
    return false;
  }

  // Antwort lesen
  std::vector<uint8_t> buffer(4096, 0);
  size_t bytes_transferred = asio::read(*impl_->socket, asio::buffer(buffer), ec);
  if (ec) {
    hh::get_logger().error("AdminClient: Receive failed: {}", ec.message());
    return false;
  }

  response = std::string(reinterpret_cast<char*>(buffer.data()), bytes_transferred);
  return true;
}

bool AdminClient::get_users(std::vector<std::string>& users) {
  std::string response;
  if (!send_command("get_users", {}, response)) {
    return false;
  }

  // Antwort parsen (eine User-ID pro Zeile)
  std::stringstream ss(response);
  std::string line;
  while (std::getline(ss, line)) {
    if (!line.empty() && line[0] != 'E') { // Keine ERROR-Zeilen
      users.push_back(line);
    }
  }

  return true;
}

bool AdminClient::get_channels(std::vector<std::string>& channels) {
  std::string response;
  if (!send_command("get_channels", {}, response)) {
    return false;
  }

  // Antwort parsen
  std::stringstream ss(response);
  std::string line;
  while (std::getline(ss, line)) {
    if (!line.empty() && line[0] != 'E') {
      channels.push_back(line);
    }
  }

  return true;
}

bool AdminClient::get_metrics(std::string& metrics) {
  return send_command("get_metrics", {}, metrics);
}

bool AdminClient::ban_user(uint32_t user_id, const std::string& reason) {
  std::map<std::string, std::string> args;
  args["user_id"] = std::to_string(user_id);
  args["reason"] = reason;
  
  std::string response;
  return send_command("ban_user", args, response);
}

bool AdminClient::kick_user(uint32_t user_id, const std::string& reason) {
  std::map<std::string, std::string> args;
  args["user_id"] = std::to_string(user_id);
  args["reason"] = reason;
  
  std::string response;
  return send_command("kick_user", args, response);
}

bool AdminClient::set_user_muted(uint32_t user_id, bool muted) {
  std::map<std::string, std::string> args;
  args["user_id"] = std::to_string(user_id);
  args["muted"] = muted ? "1" : "0";
  
  std::string response;
  return send_command("set_user_muted", args, response);
}

bool AdminClient::set_user_deafened(uint32_t user_id, bool deafened) {
  std::map<std::string, std::string> args;
  args["user_id"] = std::to_string(user_id);
  args["deafened"] = deafened ? "1" : "0";
  
  std::string response;
  return send_command("set_user_deafened", args, response);
}

} // namespace hh::server

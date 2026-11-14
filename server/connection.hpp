// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hh::server {

// Verbindungsstatus
enum class ConnectionState {
  CONNECTED,
  HELLO_SENT,
  AUTHENTICATING,
  AUTHENTICATED,
  DISCONNECTED
};

// Repräsentiert eine Client-Verbindung
class Connection : public std::enable_shared_from_this<Connection> {
public:
  using MessageHandler = std::function<void(uint16_t type, std::span<const uint8_t> payload)>;

  explicit Connection(asio::ssl::stream<asio::ip::tcp::socket>& socket);
  ~Connection();

  // Verbindung starten (TLS-Handshake, dann lesen)
  void start(MessageHandler handler);

  // Nachricht senden (mit Framing)
  void send(uint16_t type, std::span<const uint8_t> payload);

  // Verbindung schließen
  void close();

  // Status
  ConnectionState state() const { return state_; }
  uint32_t user_id() const { return user_id_; }
  void set_user_id(uint32_t id) { user_id_ = id; }
  const std::string& remote_address() const { return remote_address_; }

private:
  void do_read_header();
  void do_read_body();
  void on_handshake(std::error_code ec);

  asio::ssl::stream<asio::ip::tcp::socket>& socket_;
  ConnectionState state_ = ConnectionState::CONNECTED;
  uint32_t user_id_ = 0;
  std::string remote_address_;
  MessageHandler message_handler_;

  // Puffer für lesende Operationen
  std::array<uint8_t, 4096> read_buffer_;
  enum class ReadState { HEADER, BODY } read_state_ = ReadState::HEADER;
  uint16_t message_type_ = 0;
  uint32_t message_length_ = 0;
  std::vector<uint8_t> message_body_;
};

} // namespace hh::server

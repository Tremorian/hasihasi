// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/connection.hpp"

#include "hh/proto/framing.hpp"
#include "hh/logging.hpp"

#include <asio/use_type.hpp>

namespace hh::server {

Connection::Connection(asio::ssl::stream<asio::ip::tcp::socket>& socket)
  : socket_(socket) {
}

Connection::~Connection() = default;

void Connection::start(MessageHandler handler) {
  message_handler_ = std::move(handler);

  // Remote-Adresse ermitteln
  auto& tcp_socket = socket_.next_layer();
  std::error_code ec;
  auto endpoint = tcp_socket.remote_endpoint(ec);
  if (!ec) {
    remote_address_ = endpoint.address().to_string();
  }

  // TLS-Handshake
  socket_.async_handshake(asio::ssl::stream_base::server,
                          [self = shared_from_this()](std::error_code ec) {
    self->on_handshake(ec);
  });
}

void Connection::send(uint16_t type, std::span<const uint8_t> payload) {
  auto frame = hh::proto::FrameBuilder::build(type, payload);

  asio::async_write(socket_,
                    asio::buffer(frame.data(), frame.size()),
                    [self = shared_from_this()](std::error_code ec, size_t) {
    if (ec) {
      hh::get_logger().warn("Send failed for connection {}: {}",
                            self->remote_address_, ec.message());
    }
  });
}

void Connection::close() {
  std::error_code ec;
  socket_.close(ec);
}

void Connection::do_read_header() {
  asio::async_read(socket_,
                   asio::buffer(read_buffer_, 2),
                   [self = shared_from_this()](std::error_code ec, size_t) {
    if (ec) {
      hh::get_logger().warn("Read header failed: {}", ec.message());
      return;
    }

    // Header lesen: [u16 type][u32 length]
    self->message_type_ = static_cast<uint16_t>(
      (static_cast<uint16_t>(self->read_buffer_[0]) << 8) |
      static_cast<uint16_t>(self->read_buffer_[1]));

    self->do_read_body();
  });
}

void Connection::do_read_body() {
  // Länge lesen (4 Bytes)
  asio::async_read(socket_,
                   asio::buffer(read_buffer_, 4),
                   [self = shared_from_this()](std::error_code ec, size_t) {
    if (ec) {
      hh::get_logger().warn("Read length failed: {}", ec.message());
      return;
    }

    self->message_length_ = static_cast<uint32_t>(
      (static_cast<uint32_t>(self->read_buffer_[0]) << 24) |
      (static_cast<uint32_t>(self->read_buffer_[1]) << 16) |
      (static_cast<uint32_t>(self->read_buffer_[2]) << 8) |
      static_cast<uint32_t>(self->read_buffer_[3]));

    // Puffer für Body allozieren
    self->message_body_.resize(self->message_length_);

    // Body lesen
    asio::async_read(self->socket_,
                     asio::buffer(self->message_body_),
                     [self = shared_from_this()](std::error_code ec, size_t) {
      if (ec) {
        hh::get_logger().warn("Read body failed: {}", ec.message());
        return;
      }

      // Handler aufrufen
      if (self->message_handler_) {
        self->message_handler_(self->message_type_, self->message_body_);
      }

      // Nächste Nachricht lesen
      self->do_read_header();
    });
  });
}

void Connection::on_handshake(std::error_code ec) {
  if (ec) {
    hh::get_logger().error("TLS handshake failed: {}", ec.message());
    return;
  }

  hh::get_logger().info("TLS handshake successful for {}", remote_address_);
  state_ = ConnectionState::HELLO_SENT;

  // Mit Lesen beginnen
  do_read_header();
}

} // namespace hh::server

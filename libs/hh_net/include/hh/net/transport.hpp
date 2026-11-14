// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <asio/ts/internet.hpp>
#include <memory>
#include <string>
#include <functional>

namespace hh::net {

// Basis-TCP-Verbindung über TLS
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  using Handler = std::function<void(std::error_code, std::span<const uint8_t>)>;

  explicit TcpConnection(asio::io_context& ioc);

  asio::ip::tcp::socket& socket();
  void connect(const std::string& host, uint16_t port, Handler on_connect);
  void send(std::span<const uint8_t> data);
  void close();

private:
  void do_read();
  void on_connect_impl(std::error_code ec);
  void on_read(std::error_code ec, size_t bytes_transferred);

  asio::io_context& ioc_;
  asio::ip::tcp::socket socket_;
  std::array<uint8_t, 4096> read_buffer_;
  Handler handler_;
};

// UDP-Socket für Voice-Stream
class UdpVoiceSocket {
public:
  explicit UdpVoiceSocket(asio::io_context& ioc);

  void open(const std::string& address, uint16_t port);
  void send(std::span<const uint8_t> data, const asio::ip::udp::endpoint& target);
  void async_receive(std::function<void(std::error_code, std::span<const uint8_t>)> handler);

private:
  asio::io_context& ioc_;
  asio::ip::udp::socket socket_;
};

} // namespace hh::net

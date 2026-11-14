// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/net/transport.hpp"

namespace hh::net {

TcpConnection::TcpConnection(asio::io_context& ioc)
  : ioc_(ioc)
  , socket_(ioc) {
}

asio::ip::tcp::socket& TcpConnection::socket() {
  return socket_;
}

void TcpConnection::connect(const std::string& host, uint16_t port, Handler on_connect) {
  // TODO: TLS-Handshake implementieren
  asio::ip::tcp::resolver resolver(ioc_);
  auto endpoints = resolver.resolve(host, std::to_string(port));
  asio::async_connect(socket_, endpoints,
    [self = shared_from_this(), on_connect = std::move(on_connect)]
    (std::error_code ec, const asio::ip::tcp::endpoint&) {
      on_connect(ec, {});
    });
}

void TcpConnection::send(std::span<const uint8_t> data) {
  // TODO: async_write mit Framing
}

void TcpConnection::close() {
  std::error_code ec;
  socket_.close(ec);
}

void TcpConnection::do_read() {
  // TODO: async_read
}

void TcpConnection::on_connect_impl(std::error_code ec) {
  // TODO
}

void TcpConnection::on_read(std::error_code ec, size_t bytes_transferred) {
  // TODO
}

UdpVoiceSocket::UdpVoiceSocket(asio::io_context& ioc)
  : ioc_(ioc)
  , socket_(ioc, asio::ip::udp::v4()) {
}

void UdpVoiceSocket::open(const std::string& address, uint16_t port) {
  // TODO
}

void UdpVoiceSocket::send(std::span<const uint8_t> data, const asio::ip::udp::endpoint& target) {
  // TODO: UDP-Voice-Paket versenden
}

void UdpVoiceSocket::async_receive(std::function<void(std::error_code, std::span<const uint8_t>)> handler) {
  // TODO
}

} // namespace hh::net

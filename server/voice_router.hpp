// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace hh::server {

// UDP-Voice-Router: empfängt Voice-Pakete und leitet sie an Ziel-Clients weiter
class VoiceRouter {
public:
  explicit VoiceRouter(asio::io_context& ioc);
  ~VoiceRouter();

  // UDP-Socket öffnen (Voice-Port)
  bool open(uint16_t port);

  // Voice-Paket an Ziel weiterleiten
  // target_endpoint: Ziel-UDP-Endpoint
  // data: verschlüsseltes Voice-Paket
  void route_to(const asio::ip::udp::endpoint& target_endpoint,
                std::span<const uint8_t> data);

  // Alle verbundenen Clients benachrichtigen (für Channel-Broadcast)
  using ClientEndpoint = std::pair<asio::ip::udp::endpoint, uint32_t>;
  void route_to_all(const std::vector<ClientEndpoint>& targets,
                    std::span<const uint8_t> data);

  // Remote-Adresse des Sockets
  asio::ip::udp::endpoint local_endpoint() const { return socket_.local_endpoint(); }

private:
  void do_receive();
  void on_receive(std::error_code ec, size_t bytes_transferred);

  asio::io_context& ioc_;
  asio::ip::udp::socket socket_;
  std::array<uint8_t, 1472> receive_buffer_; // Max Opus-Paketgröße
};

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/voice_router.hpp"

#include "hh/logging.hpp"

namespace hh::server {

VoiceRouter::VoiceRouter(asio::io_context& ioc)
  : ioc_(ioc)
  , socket_(ioc, asio::ip::udp::v4()) {
}

VoiceRouter::~VoiceRouter() = default;

bool VoiceRouter::open(uint16_t port) {
  std::error_code ec;
  socket_.open(asio::ip::udp::v4(), ec);
  if (ec) {
    hh::get_logger().error("Failed to open UDP socket: {}", ec.message());
    return false;
  }

  asio::ip::udp::endpoint bind_endpoint(asio::ip::udp::v4(), port);
  socket_.bind(bind_endpoint, ec);
  if (ec) {
    hh::get_logger().error("Failed to bind UDP socket to port {}: {}", port, ec.message());
    return false;
  }

  hh::get_logger().info("Voice UDP socket bound to port {}", port);
  do_receive();
  return true;
}

void VoiceRouter::route_to(const asio::ip::udp::endpoint& target_endpoint,
                           std::span<const uint8_t> data) {
  std::error_code ec;
  socket_.async_send_to(asio::buffer(data.data(), data.size()), target_endpoint,
                        [target_endpoint](std::error_code ec, size_t) {
    if (ec) {
      hh::get_logger().debug("Voice route failed to {}: {}",
                             target_endpoint.address().to_string(),
                             target_endpoint.port());
    }
  });
}

void VoiceRouter::route_to_all(const std::vector<ClientEndpoint>& targets,
                               std::span<const uint8_t> data) {
  for (const auto& [endpoint, user_id] : targets) {
    route_to(endpoint, data);
  }
}

void VoiceRouter::do_receive() {
  socket_.async_receive_from(
    asio::buffer(receive_buffer_),
    [](std::error_code, size_t) {
      // Voice-Daten werden hier empfangen und an den Control-Kanal weitergeleitet
      // (Implementierung in M3+)
    });
}

void VoiceRouter::on_receive(std::error_code ec, size_t bytes_transferred) {
  if (ec) {
    hh::get_logger().warn("Voice receive failed: {}", ec.message());
    return;
  }

  // Voice-Daten verarbeiten (Implementierung in M3+)
  (void)bytes_transferred;
}

} // namespace hh::server

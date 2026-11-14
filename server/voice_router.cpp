// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/voice_router.hpp"

#include "hh/logging.hpp"

#include <random>

namespace hh::server {

VoiceRouter::VoiceRouter(asio::io_context& ioc)
  : ioc_(ioc)
  , socket_(ioc, asio::ip::udp::v4())
  , buffer_pool_(std::make_unique<BufferPool>()) {
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

bool VoiceRouter::initialize_crypto(const std::array<uint8_t, VoiceCrypto::KEY_SIZE>& key) {
  crypto_ = std::make_unique<VoiceCrypto>();
  if (!crypto_->initialize(key)) {
    hh::get_logger().error("Failed to initialize voice crypto");
    return false;
  }

  // Schlüssel-Rotation periodisch starten
  auto rotation_timer = std::make_shared<asio::steady_timer>(ioc_);
  auto start_rotation = [this, rotation_timer]() {
    rotation_timer->expires_from_now(std::chrono::milliseconds(crypto_->config().rotation_interval_ms));
    rotation_timer->async_wait([this, rotation_timer](std::error_code ec) {
      if (!ec) {
        rotate_key();
        start_rotation();
      }
    });
  };
  start_rotation();

  hh::get_logger().info("Voice crypto initialized with key rotation");
  return true;
}

bool VoiceRouter::open_tcp_fallback(uint16_t port) {
  std::error_code ec;
  
  // TCP-Acceptor öffnen
  tcp_acceptor_.open(asio::ip::tcp::v4(), ec);
  if (ec) {
    hh::get_logger().error("Failed to open TCP fallback socket: {}", ec.message());
    return false;
  }

  // Port wiederverwenden
  tcp_acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
  if (ec) {
    hh::get_logger().warn("Failed to set TCP reuse_address: {}", ec.message());
  }

  // Binden
  asio::ip::tcp::endpoint bind_endpoint(asio::ip::tcp::v4(), port);
  tcp_acceptor_.bind(bind_endpoint, ec);
  if (ec) {
    hh::get_logger().error("Failed to bind TCP fallback socket to port {}: {}", port, ec.message());
    return false;
  }

  // Hören
  tcp_acceptor_.listen(asio::socket_base::max_listen_connections, ec);
  if (ec) {
    hh::get_logger().error("Failed to listen on TCP fallback socket: {}", ec.message());
    return false;
  }

  tcp_fallback_active_ = true;
  hh::get_logger().info("TCP fallback socket bound to port {}", port);
  return true;
}

std::array<uint8_t, VoiceCrypto::NONCE_SIZE> VoiceRouter::generate_nonce_deterministic(
    uint64_t sequence, uint8_t session_id) {
  std::array<uint8_t, VoiceCrypto::NONCE_SIZE> nonce = {};
  
  // Nonce aus Sequenznummer und Session-ID generieren (deterministisch)
  // Struktur: [session_id (1B)][sequence (7B)]
  nonce[0] = session_id;
  
  // Sequenznummer in 7 Bytes umwandeln (Big-Endian)
  for (int i = 0; i < 7; ++i) {
    nonce[1 + i] = static_cast<uint8_t>((sequence >> (56 - i * 8)) & 0xFF);
  }
  
  return nonce;
}

void VoiceRouter::set_crypto_config(const VoiceCrypto::Config& config) {
  if (crypto_) {
    crypto_->set_config(config);
  }
}

void VoiceRouter::route_to(const asio::ip::udp::endpoint& target_endpoint,
                           std::span<const uint8_t> data,
                           uint8_t session_id,
                           uint8_t target) {
  if (!crypto_) {
    hh::get_logger().error("Voice crypto not initialized");
    return;
  }

  // Deterministische Sequenznummer basierend auf Zeit und Session
  static thread_local uint64_t sequence_counter = 0;
  sequence_counter++;
  uint64_t sequence = sequence_counter;

  // Verschlüsseln mit deterministischem Nonce
  auto nonce = generate_nonce_deterministic(sequence, session_id);
  auto result = crypto_->encrypt_with_nonce(data, nonce, session_id, target);
  if (!result.success) {
    hh::get_logger().error("Voice encryption failed");
    return;
  }

  // Puffer aus Pool holen
  auto buffer = buffer_pool_->acquire();

  // Verschlüsseltes Paket kopieren
  std::memcpy(buffer.data(), data.data(), result.output_size);

  // Senden
  std::error_code ec;
  socket_.async_send_to(asio::buffer(buffer.data(), result.output_size), target_endpoint,
                        [this, target_endpoint, buffer, result](std::error_code ec, size_t bytes_sent) {
    if (ec) {
      hh::get_logger().debug("Voice route failed to {}: {}",
                             target_endpoint.address().to_string(),
                             target_endpoint.port());
    } else {
      stats_.packets_sent++;
      stats_.bytes_sent += bytes_sent;
      buffer_pool_->release(std::move(buffer));
    }
  });
}

void VoiceRouter::route_to_all(const std::vector<ClientEndpoint>& targets,
                               std::span<const uint8_t> data,
                               uint8_t session_id) {
  for (const auto& [endpoint, user_id] : targets) {
    (void)user_id;
    route_to(endpoint, data, session_id, static_cast<uint8_t>(user_id & 0xFF));
  }
}

void VoiceRouter::rotate_key() {
  if (crypto_) {
    crypto_->rotate_key();
    stats_.key_rotations++;
    hh::get_logger().info("Voice router: Key rotated");
  }
}

VoiceRouter::Stats VoiceRouter::get_stats() const {
  Stats s = stats_;
  if (crypto_) {
    s.crypto_stats = crypto_->get_stats();
  }
  return s;
}

void VoiceRouter::reset_stats() {
  stats_ = {};
  if (crypto_) {
    crypto_->reset_stats();
  }
}

void VoiceRouter::set_voice_packet_handler(VoicePacketHandler handler) {
  voice_packet_handler_ = std::move(handler);
}

void VoiceRouter::do_receive() {
  auto buffer = buffer_pool_->acquire();

  socket_.async_receive_from(
    asio::buffer(buffer.data(), buffer.size()),
    [this, buffer](std::error_code ec, size_t bytes_transferred) {
      on_receive(ec, bytes_transferred, std::move(buffer));
    });
}

void VoiceRouter::on_receive(std::error_code ec, size_t bytes_transferred, std::vector<uint8_t> buffer) {
  if (ec) {
    hh::get_logger().warn("Voice receive failed: {}", ec.message());
    buffer_pool_->release(std::move(buffer));
    return;
  }

  if (bytes_transferred == 0) {
    buffer_pool_->release(std::move(buffer));
    do_receive();
    return;
  }

  stats_.packets_received++;
  stats_.bytes_received += bytes_transferred;

  // Handler aufrufen (wenn gesetzt)
  if (voice_packet_handler_) {
    // Endpoint wird später gesetzt, hier nur Daten weitergeben
    voice_packet_handler_(asio::ip::udp::endpoint(), std::span(buffer.data(), bytes_transferred));
  }

  buffer_pool_->release(std::move(buffer));
  do_receive();
}

} // namespace hh::server

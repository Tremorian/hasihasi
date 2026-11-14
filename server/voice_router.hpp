// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "server/voice_crypto.hpp"

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <queue>

namespace hh::server {

// UDP-Voice-Router: empfängt Voice-Pakete und leitet sie an Ziel-Clients weiter
// Mit AEAD-Verschlüsselung (AES-256-GCM), Pufferpools und Schlüssel-Rotation
class VoiceRouter {
public:
  explicit VoiceRouter(asio::io_context& ioc);
  ~VoiceRouter();

  // UDP-Socket öffnen (Voice-Port)
  bool open(uint16_t port);

  // Verschlüsselung initialisieren
  bool initialize_crypto(const std::array<uint8_t, VoiceCrypto::KEY_SIZE>& key);
  void set_crypto_config(const VoiceCrypto::Config& config);

  // Voice-Paket an Ziel weiterleiten (verschlüsselt)
  // target_endpoint: Ziel-UDP-Endpoint
  // data: rohes Opus-Paket
  // session_id: Session-ID des Sprechers
  // target: Ziel-ID (0xFF = Broadcast)
  void route_to(const asio::ip::udp::endpoint& target_endpoint,
                std::span<const uint8_t> data,
                uint8_t session_id,
                uint8_t target = 0xFF);

  // Alle verbundenen Clients benachrichtigen (für Channel-Broadcast)
  using ClientEndpoint = std::pair<asio::ip::udp::endpoint, uint32_t>;
  void route_to_all(const std::vector<ClientEndpoint>& targets,
                    std::span<const uint8_t> data,
                    uint8_t session_id);

  // Schlüssel rotieren
  void rotate_key();

  // Remote-Adresse des Sockets
  asio::ip::udp::endpoint local_endpoint() const { return socket_.local_endpoint(); }

  // Statistik
  struct Stats {
    size_t packets_sent;
    size_t packets_received;
    size_t bytes_sent;
    size_t bytes_received;
    size_t key_rotations;
    VoiceCrypto::Stats crypto_stats;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using VoicePacketHandler = std::function<void(const asio::ip::udp::endpoint& sender,
                                                 std::span<const uint8_t> encrypted_data)>;
  void set_voice_packet_handler(VoicePacketHandler handler);

private:
  void do_receive();
  void on_receive(std::error_code ec, size_t bytes_transferred);

  // Puffer-Pool für Voice-Pakete (kein Heap pro Paket)
  struct BufferPool {
    static constexpr size_t POOL_SIZE = 1000;
    static constexpr size_t BUFFER_SIZE = VoiceCrypto::MAX_PACKET_SIZE;

    std::queue<std::vector<uint8_t>> pool_;

    BufferPool() {
      for (size_t i = 0; i < POOL_SIZE; ++i) {
        pool_.emplace(BUFFER_SIZE, 0);
      }
    }

    std::vector<uint8_t> acquire() {
      if (pool_.empty()) {
        return std::vector<uint8_t>(BUFFER_SIZE, 0);
      }
      auto buf = std::move(pool_.front());
      pool_.pop();
      return buf;
    }

    void release(std::vector<uint8_t> buf) {
      if (pool_.size() < POOL_SIZE) {
        buf.resize(BUFFER_SIZE, 0);
        pool_.push(std::move(buf));
      }
    }
  };

  asio::io_context& ioc_;
  asio::ip::udp::socket socket_;
  std::unique_ptr<BufferPool> buffer_pool_;
  std::unique_ptr<VoiceCrypto> crypto_;

  Stats stats_;
  VoicePacketHandler voice_packet_handler_;
};

} // namespace hh::server

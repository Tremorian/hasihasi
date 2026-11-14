// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/session.hpp"

#include "hh/logging.hpp"

#include <asio/use_type.hpp>

namespace hh::client {

Session::Session()
  : ssl_context_(asio::ssl::context::sslv23)
  , socket_(nullptr)
  , resolver_(std::make_unique<asio::ip::tcp::resolver>(ioc_))
  , stats_timer_(ioc_) {
  // Zertifizierungsprüfung deaktivieren (für Testzwecke)
  ssl_context_.set_verify_mode(asio::ssl::verify_none);

  // Statistik-Timer starten
  stats_timer_.expires_after(std::chrono::seconds(1));
  stats_timer_.async_wait([this](std::error_code ec) {
    if (!ec) {
      update_network_stats();
      stats_timer_.expires_after(std::chrono::seconds(1));
      stats_timer_.async_wait([this](std::error_code ec) {
        if (!ec) {
          // Rekursiver Aufruf für kontinuierliche Updates
        }
      });
    }
  });
}

Session::~Session() {
  disconnect();
}

bool Session::connect(const std::string& host, uint16_t port) {
  host_ = host;
  port_ = port;

  hh::get_logger().info("Verbinde mit {}:{}...", host, port);

  // Asio-IO-Context in einem Thread starten
  std::thread([this]() {
    ioc_.run();
  }).detach();

  // Resolver auflösen
  auto self = shared_from_this(); // Brauchen wir nicht, aber für async
  resolver_->async_resolve(host, std::to_string(port_),
                           [this](std::error_code ec, asio::ip::tcp::resolver::results_type results) {
    if (ec) {
      hh::get_logger().error("Resolver failed: {}", ec.message());
      if (connection_callback_) {
        connection_callback_(false, "Resolver fehlgeschlagen: " + ec.message());
      }
      return;
    }

    // Verbindung aufbauen
    socket_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(
      ioc_, ssl_context_);

    asio::async_connect(socket_->next_layer(), results,
                        [this](std::error_code ec, asio::ip::tcp::endpoint) {
      on_connect_result(ec);
    });
  });

  return true;
}

void Session::disconnect() {
  connected_ = false;

  if (socket_) {
    std::error_code ec;
    socket_->shutdown(ec);
    socket_->close(ec);
  }

  ioc_.stop();

  hh::get_logger().info("Verbindung getrennt");
}

bool Session::authenticate(const std::string& username) {
  username_ = username;

  // Hello-Nachricht senden
  std::vector<uint8_t> payload;
  payload.resize(4 + username.size());

  // Protocol-Version (u32)
  uint32_t version = 1;
  payload[0] = (version >> 24) & 0xFF;
  payload[1] = (version >> 16) & 0xFF;
  payload[2] = (version >> 8) & 0xFF;
  payload[3] = version & 0xFF;

  // Username-Length (u32)
  uint32_t len = static_cast<uint32_t>(username.size());
  payload[4] = (len >> 24) & 0xFF;
  payload[5] = (len >> 16) & 0xFF;
  payload[6] = (len >> 8) & 0xFF;
  payload[7] = len & 0xFF;

  // Username
  std::copy(username.begin(), username.end(), payload.begin() + 8);

  send_message(0, payload); // MSG_HELLO

  hh::get_logger().info("Authentifiziere als: {}", username);
  return true;
}

void Session::request_channel_list() {
  send_message(4, {}); // MSG_CHANNEL_LIST
}

void Session::join_channel(uint32_t channel_id) {
  std::vector<uint8_t> payload(4);
  payload[0] = (channel_id >> 24) & 0xFF;
  payload[1] = (channel_id >> 16) & 0xFF;
  payload[2] = (channel_id >> 8) & 0xFF;
  payload[3] = channel_id & 0xFF;

  send_message(5, payload); // MSG_JOIN_CHANNEL
}

void Session::send_text(const std::string& text) {
  std::vector<uint8_t> payload(text.begin(), text.end());
  send_message(6, payload); // MSG_TEXT_MESSAGE
}

void Session::send_voice_data(std::span<const uint8_t> voice_data) {
  send_message(7, voice_data); // MSG_VOICE_DATA
}

void Session::on_connect_result(std::error_code ec) {
  if (ec) {
    hh::get_logger().error("Connect failed: {}", ec.message());
    if (connection_callback_) {
      connection_callback_(false, "Verbindung fehlgeschlagen: " + ec.message());
    }
    return;
  }

  hh::get_logger().info("TCP-Verbindung hergestellt");

  // TLS-Handshake
  socket_->async_handshake(asio::ssl::stream_base::client,
                           [this](std::error_code ec) {
    on_handshake(ec);
  });
}

void Session::on_handshake(std::error_code ec) {
  if (ec) {
    hh::get_logger().error("TLS handshake failed: {}", ec.message());
    if (connection_callback_) {
      connection_callback_(false, "TLS-Handshake fehlgeschlagen: " + ec.message());
    }
    return;
  }

  connected_ = true;
  hh::get_logger().info("TLS-Handshake erfolgreich");

  if (connection_callback_) {
    connection_callback_(true, "Verbunden");
  }

  // Lesen beginnen
  read_buffer_.resize(2);
  on_read_header({}, 0);
}

void Session::on_read_header(std::error_code ec, size_t) {
  if (ec) {
    hh::get_logger().warn("Read header failed: {}", ec.message());
    return;
  }

  // Header lesen: [u16 type][u32 length]
  message_type_ = static_cast<uint16_t>(
    (static_cast<uint16_t>(read_buffer_[0]) << 8) |
    static_cast<uint16_t>(read_buffer_[1]));

  // Länge lesen
  read_buffer_.resize(4);
  asio::async_read(*socket_,
                   asio::buffer(read_buffer_),
                   [this](std::error_code ec, size_t) {
    if (ec) {
      hh::get_logger().warn("Read length failed: {}", ec.message());
      return;
    }

    message_length_ = static_cast<uint32_t>(
      (static_cast<uint32_t>(read_buffer_[0]) << 24) |
      (static_cast<uint32_t>(read_buffer_[1]) << 16) |
      (static_cast<uint32_t>(read_buffer_[2]) << 8) |
      static_cast<uint32_t>(read_buffer_[3]));

    message_body_.resize(message_length_);
    on_read_body({}, 0);
  });
}

void Session::on_read_body(std::error_code ec, size_t) {
  (void);

  if (ec) {
    hh::get_logger().warn("Read body failed: {}", ec.message());
    return;
  }

  // Nachricht verarbeiten
  switch (message_type_) {
    case 0: handle_hello(message_body_); break;
    case 4: handle_channel_list(message_body_); break;
    case 5: handle_user_list(message_body_); break;
    case 6: handle_text_message(message_body_); break;
    case 7: handle_voice_data(message_body_); break;
    default:
      hh::get_logger().warn("Unknown message type: {}", message_type_);
      break;
  }

  // Nächste Nachricht lesen
  read_buffer_.resize(2);
  on_read_header({}, 0);
}

void Session::send_message(uint16_t type, std::span<const uint8_t> payload) {
  if (!connected_ || !socket_) {
    hh::get_logger().warn("Cannot send message: not connected");
    return;
  }

  // Framing: [u16 type][u32 length][payload]
  std::vector<uint8_t> frame;
  frame.resize(2 + 4 + payload.size());

  // Type
  frame[0] = (type >> 8) & 0xFF;
  frame[1] = type & 0xFF;

  // Length
  uint32_t len = static_cast<uint32_t>(payload.size());
  frame[2] = (len >> 24) & 0xFF;
  frame[3] = (len >> 16) & 0xFF;
  frame[4] = (len >> 8) & 0xFF;
  frame[5] = len & 0xFF;

  // Payload
  std::copy(payload.begin(), payload.end(), frame.begin() + 6);

  // Asynchronously write
  asio::async_write(*socket_,
                    asio::buffer(frame),
                    [this](std::error_code ec, size_t) {
      if (ec) {
        hh::get_logger().error("Send failed: {}", ec.message());
      }
    });
}

void Session::handle_hello(std::span<const uint8_t> payload) {
  (void)payload;
  hh::get_logger().info("Server Hello received");
}

void Session::handle_channel_list(std::span<const uint8_t> payload) {
  // Einfache Verarbeitung: Kanal-IDs und Namen extrahieren
  // TODO: Protobuf-parsing
  (void)payload;

  // Platzhalter: Leere Liste senden
  if (channel_update_callback_) {
    channel_update_callback_({});
  }
}

void Session::handle_user_list(std::span<const uint8_t> payload) {
  (void)payload;
  // TODO: Protobuf-parsing
  if (user_update_callback_) {
    user_update_callback_(0, {});
  }
}

void Session::handle_text_message(std::span<const uint8_t> payload) {
  std::string text(payload.begin(), payload.end());
  hh::get_logger().info("Text message received: {}", text);

  if (text_message_callback_) {
    text_message_callback_(0, text);
  }
}

void Session::handle_voice_data(std::span<const uint8_t> payload) {
  if (voice_data_callback_) {
    voice_data_callback_(payload);
  }
}

void Session::update_network_stats() {
  // Vereinfachte Statistik (in echter Implementierung: Asio-Metrics)
  rtt_ms_ = 5.0; // Platzhalter
  jitter_ms_ = 1.0;
  loss_percent_ = 0.0;
  bitrate_kbit_ = 40;
  latency_ms_ = 10;
}

} // namespace hh::client

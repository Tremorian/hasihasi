// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <cstdint>

namespace hh::client {

// Session-Logik mit echter Asio/TLS-Netzwerk-Integration
class Session {
public:
  using ChannelList = std::vector<std::pair<uint32_t, std::string>>;
  using UserList = std::vector<std::pair<uint32_t, std::string>>;
  using ConnectionCallback = std::function<void(bool success, const std::string& message)>;
  using ChannelUpdateCallback = std::function<void(const ChannelList& channels)>;
  using UserUpdateCallback = std::function<void(uint32_t channel_id, const UserList& users)>;
  using VoiceDataCallback = std::function<void(std::span<const uint8_t> voice_data)>;
  using TextMessageCallback = std::function<void(uint32_t from_user_id, const std::string& text)>;

  explicit Session();
  ~Session();

  // Verbindung zum Server
  bool connect(const std::string& host, uint16_t port);
  void disconnect();

  // Authentifizierung
  bool authenticate(const std::string& username);

  // Status
  bool is_connected() const { return connected_; }
  const std::string& username() const { return username_; }
  const std::string& host() const { return host_; }
  uint16_t port() const { return port_; }

  // Callbacks registrieren
  void set_connection_callback(ConnectionCallback cb) { connection_callback_ = std::move(cb); }
  void set_channel_update_callback(ChannelUpdateCallback cb) { channel_update_callback_ = std::move(cb); }
  void set_user_update_callback(UserUpdateCallback cb) { user_update_callback_ = std::move(cb); }
  void set_voice_data_callback(VoiceDataCallback cb) { voice_data_callback_ = std::move(cb); }
  void set_text_message_callback(TextMessageCallback cb) { text_message_callback_ = std::move(cb); }

  // Kanal-Operationen
  void request_channel_list();
  void join_channel(uint32_t channel_id);

  // Textnachricht senden
  void send_text(const std::string& text);

  // Voice-Daten senden
  void send_voice_data(std::span<const uint8_t> voice_data);

  // Statistik
  double rtt_ms() const { return rtt_ms_; }
  double jitter_ms() const { return jitter_ms_; }
  double loss_percent() const { return loss_percent_; }
  int bitrate_kbit() const { return bitrate_kbit_; }
  int latency_ms() const { return latency_ms_; }

private:
  // Asio-Handler
  void on_connect_result(std::error_code ec);
  void on_handshake(std::error_code ec);
  void on_read_header(std::error_code ec, size_t bytes_transferred);
  void on_read_body(std::error_code ec, size_t bytes_transferred);
  void on_send_voice(std::error_code ec, size_t bytes_transferred);

  // Protobuf-Nachrichten verarbeiten
  void handle_hello(std::span<const uint8_t> payload);
  void handle_channel_list(std::span<const uint8_t> payload);
  void handle_user_list(std::span<const uint8_t> payload);
  void handle_text_message(std::span<const uint8_t> payload);
  void handle_voice_data(std::span<const uint8_t> payload);

  // Framing
  void send_message(uint16_t type, std::span<const uint8_t> payload);

  // Netzwerk-Statistik aktualisieren
  void update_network_stats();

  // Asio
  asio::io_context ioc_;
  asio::ssl::context ssl_context_;
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket_;
  std::unique_ptr<asio::ip::tcp::resolver> resolver_;

  // Verbindungszustand
  bool connected_ = false;
  std::string username_;
  std::string host_;
  uint16_t port_ = 0;

  // Lese-Puffer
  std::vector<uint8_t> read_buffer_;
  uint16_t message_type_ = 0;
  uint32_t message_length_ = 0;
  std::vector<uint8_t> message_body_;

  // Statistik
  double rtt_ms_ = 0.0;
  double jitter_ms_ = 0.0;
  double loss_percent_ = 0.0;
  int bitrate_kbit_ = 0;
  int latency_ms_ = 0;

  // Callbacks
  ConnectionCallback connection_callback_;
  ChannelUpdateCallback channel_update_callback_;
  UserUpdateCallback user_update_callback_;
  VoiceDataCallback voice_data_callback_;
  TextMessageCallback text_message_callback_;

  // Timer für Statistik
  asio::steady_timer stats_timer_;
};

} // namespace hh::client

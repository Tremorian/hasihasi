// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/session.hpp"

#include "hh/logging.hpp"

namespace hh::client {

Session::Session() = default;
Session::~Session() = default;

bool Session::connect(const std::string& host, uint16_t port) {
  host_ = host;
  port_ = port;
  // TODO: Asio-Verbindung aufbauen
  connected_ = true;
  return true;
}

void Session::disconnect() {
  connected_ = false;
  on_disconnect();
}

bool Session::authenticate(const std::string& username, const std::string& certificate_path) {
  username_ = username;
  // TODO: Zertifikat laden und senden
  return true;
}

void Session::on_connect() {
  hh::get_logger().info("Verbunden mit {}:{}", host_, port_);
}

void Session::on_disconnect() {
  hh::get_logger().info("Verbindung getrennt");
}

void Session::on_auth_result() {
  hh::get_logger().info("Authentifiziert als: {}", username_);
}

} // namespace hh::client

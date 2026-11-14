// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <memory>

namespace hh::client {

// Session-Logik (ohne Qt)
class Session {
public:
  explicit Session();
  ~Session();

  // Verbindung zum Server
  bool connect(const std::string& host, uint16_t port);
  void disconnect();

  // Authentifizierung
  bool authenticate(const std::string& username, const std::string& certificate_path);

  // Status
  bool is_connected() const { return connected_; }
  const std::string& username() const { return username_; }

private:
  void on_connect();
  void on_disconnect();
  void on_auth_result();

  bool connected_ = false;
  std::string username_;
  std::string host_;
  uint16_t port_ = 0;
};

} // namespace hh::client

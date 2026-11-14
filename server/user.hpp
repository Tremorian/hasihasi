// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hh::server {

// Vorwärtsdeklaration
class Connection;

// Repräsentiert einen authentifizierten Nutzer
class User {
public:
  User(uint32_t id, const std::string& username,
       const std::shared_ptr<Connection>& connection);

  // Grundlegende Eigenschaften
  uint32_t id() const { return id_; }
  const std::string& username() const { return username_; }
  const std::shared_ptr<Connection>& connection() const { return connection_; }
  uint32_t channel_id() const { return channel_id_; }

  // Zertifikats-Fingerprint
  const std::string& cert_fingerprint() const { return cert_fingerprint_; }
  void set_cert_fingerprint(const std::string& fingerprint) { cert_fingerprint_ = fingerprint; }
  bool has_cert() const { return !cert_fingerprint_.empty(); }

  // Status
  void set_muted(bool muted) { muted_ = muted; }
  bool is_muted() const { return muted_; }
  void set_deafened(bool deafened) { deafened_ = deafened; }
  bool is_deafened() const { return deafened_; }
  void set_suppressing(bool suppressing) { suppressing_ = suppressing; }
  bool is_suppressing() const { return suppressing_; }

  // Admin-Status
  bool is_admin() const { return is_admin_; }
  void set_admin(bool admin) { is_admin_ = admin; }

  // Netzwerk
  asio::ip::udp::endpoint udp_endpoint() const { return udp_endpoint_; }
  void set_udp_endpoint(asio::ip::udp::endpoint endpoint) { udp_endpoint_ = endpoint; }

private:
  uint32_t id_;
  std::string username_;
  std::shared_ptr<Connection> connection_;
  uint32_t channel_id_ = 0;
  bool muted_ = false;
  bool deafened_ = false;
  bool suppressing_ = false;
  bool is_admin_ = false;
  std::string cert_fingerprint_;
  asio::ip::udp::endpoint udp_endpoint_;
};

// Nutzer-Verwaltung
class UserManager {
public:
  // Nutzer erstellen
  std::shared_ptr<User> create_user(uint32_t id, const std::string& username,
                                     const std::shared_ptr<Connection>& connection);

  // Nutzer finden
  std::shared_ptr<User> find_user(uint32_t id) const;
  std::shared_ptr<User> find_user_by_name(const std::string& name) const;

  // Nutzer entfernen
  bool remove_user(uint32_t id);

  // Alle Nutzer
  std::vector<std::shared_ptr<User>> all_users() const;
  size_t user_count() const { return users_.size(); }

private:
  std::map<uint32_t, std::shared_ptr<User>> users_;
  std::map<std::string, std::shared_ptr<User>> users_by_name_;
};

} // namespace hh::server

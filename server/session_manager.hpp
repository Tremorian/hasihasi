// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "server/channel.hpp"
#include "server/user.hpp"
#include "server/voice_router.hpp"
#include "server/acl.hpp"

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace hh::server {

// Erweiterte Kanal-Verwaltung mit Priority-Speech
class MultiChannelManager {
public:
  MultiChannelManager();

  // Kanal erstellen (erweitert)
  std::shared_ptr<Channel> create_channel(const std::string& name,
                                           uint32_t max_users = 0,
                                           bool temporary = false,
                                           const std::string& description = "",
                                           bool priority_speech = false);

  // Kanal löschen
  bool delete_channel(uint32_t channel_id);

  // Kanal finden
  std::shared_ptr<Channel> find_channel(uint32_t channel_id) const;

  // Alle Kanäle
  std::vector<std::shared_ptr<Channel>> all_channels() const;

  // Nutzer zuweisen
  bool assign_user(uint32_t user_id, uint32_t channel_id);

  // Priority-Speech
  void enable_priority_speech(uint32_t user_id, bool enabled);
  bool is_priority_speaker(uint32_t user_id) const;

  // Sprecher-Liste für Kanal
  std::vector<std::pair<uint32_t, std::string>> get_speakers(uint32_t channel_id) const;

  // Kanal-Statistik
  struct ChannelStats {
    uint32_t channel_id;
    std::string name;
    size_t user_count;
    size_t speaker_count;
    size_t priority_speakers;
  };
  std::vector<ChannelStats> get_channel_stats() const;

private:
  ChannelTree channel_tree_;
  std::map<uint32_t, bool> priority_speakers_; // user_id -> enabled
};

// Server-Logik für Deaf/Mute/Priority-Speech
class ServerSessionManager {
public:
  using MessageHandler = std::function<void(uint32_t user_id, uint16_t type,
                                             std::span<const uint8_t> payload)>;

  explicit ServerSessionManager(asio::io_context& ioc);

  // Nutzer registrieren
  std::shared_ptr<User> register_user(uint32_t id, const std::string& username,
                                       const std::shared_ptr<Connection>& connection);

  // Nutzer mit Zertifikat registrieren
  std::shared_ptr<User> register_user_with_cert(uint32_t id, const std::string& username,
                                                 const std::string& cert_fingerprint,
                                                 const std::shared_ptr<Connection>& connection);

  // Nutzer über Zertifikat finden
  std::shared_ptr<User> find_user_by_cert(const std::string& cert_fingerprint) const;

  // Nutzer entfernen
  void unregister_user(uint32_t user_id);

  // Nutzer finden
  std::shared_ptr<User> find_user(uint32_t id) const;

  // Status setzen
  void set_user_muted(uint32_t user_id, bool muted);
  void set_user_deafened(uint32_t user_id, bool deafened);
  void set_user_suppressing(uint32_t user_id, bool suppressing);

  // Priority-Speech
  void enable_priority_speech(uint32_t user_id, bool enabled);
  bool is_priority_speaker(uint32_t user_id) const;

  // Kanal-Wechsel
  bool switch_channel(uint32_t user_id, uint32_t new_channel_id);

  // Voice-Weiterleitung (vereinfacht)
  void route_voice(uint32_t from_user_id, std::span<const uint8_t> voice_data);

  // ACL-Verwaltung
  std::shared_ptr<AclGroup> create_acl_group(const std::string& name, bool inherits = false);
  bool delete_acl_group(uint32_t group_id);
  std::shared_ptr<AclGroup> find_acl_group(uint32_t group_id) const;
  bool check_user_right(uint32_t user_id, uint32_t channel_id, ChannelRight right) const;
  void set_user_admin(uint32_t user_id, bool admin);
  bool is_user_admin(uint32_t user_id) const;

  // Statistik
  size_t user_count() const { return user_manager_.user_count(); }
  size_t channel_count() const { return multi_channel_.all_channels().size(); }
  size_t active_speakers() const;

  // Callbacks
  void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }

private:
  MultiChannelManager multi_channel_;
  UserManager user_manager_;
  VoiceRouter voice_router_;
  AclManager acl_manager_;
  MessageHandler message_handler_;
};

} // namespace hh::server

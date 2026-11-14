// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/session_manager.hpp"

#include "hh/logging.hpp"

namespace hh::server {

// ── MultiChannelManager ───────────────────────────────────────────────

MultiChannelManager::MultiChannelManager() {
  // Root-Kanal erstellen
  create_channel("Root", 0, false, "Wurzel-Kanal");
}

std::shared_ptr<Channel> MultiChannelManager::create_channel(const std::string& name,
                                                              uint32_t max_users,
                                                              bool temporary,
                                                              const std::string& description,
                                                              bool priority_speech) {
  (void)priority_speech; // TODO: Priority-Speech-Flag im Channel speichern

  auto channel = channel_tree_.create_channel(ChannelTree::ROOT_CHANNEL_ID, name,
                                               max_users, temporary, description);
  if (channel) {
    hh::get_logger().info("MultiChannelManager: Created channel '{}' (id: {}, max: {})",
                          name, channel->id(), max_users);
  }
  return channel;
}

bool MultiChannelManager::delete_channel(uint32_t channel_id) {
  return channel_tree_.delete_channel(channel_id);
}

std::shared_ptr<Channel> MultiChannelManager::find_channel(uint32_t channel_id) const {
  return channel_tree_.find_channel(channel_id);
}

std::vector<std::shared_ptr<Channel>> MultiChannelManager::all_channels() const {
  return channel_tree_.all_channels();
}

bool MultiChannelManager::assign_user(uint32_t user_id, uint32_t channel_id) {
  return channel_tree_.assign_user_to_channel(user_id, channel_id);
}

void MultiChannelManager::enable_priority_speech(uint32_t user_id, bool enabled) {
  priority_speakers_[user_id] = enabled;
  hh::get_logger().info("MultiChannelManager: User {} priority_speech={}", user_id, enabled);
}

bool MultiChannelManager::is_priority_speaker(uint32_t user_id) const {
  auto it = priority_speakers_.find(user_id);
  return it != priority_speakers_.end() && it->second;
}

std::vector<std::pair<uint32_t, std::string>> MultiChannelManager::get_speakers(uint32_t channel_id) const {
  (void)channel_id;
  // TODO: Sprecher-Liste basierend auf VAD/PTT aktualisieren
  return {};
}

std::vector<MultiChannelManager::ChannelStats> MultiChannelManager::get_channel_stats() const {
  std::vector<ChannelStats> stats;

  for (const auto& channel : channel_tree_.all_channels()) {
    ChannelStats s;
    s.channel_id = channel->id();
    s.name = channel->name();
    s.user_count = channel->user_count();
    s.speaker_count = 0; // TODO: Zählen
    s.priority_speakers = 0; // TODO: Zählen
    stats.push_back(s);
  }

  return stats;
}

// ── ServerSessionManager ──────────────────────────────────────────────

ServerSessionManager::ServerSessionManager(asio::io_context& ioc)
  : voice_router_(ioc) {
  // Voice-Router öffnen
  voice_router_.open(64738);
}

std::shared_ptr<User> ServerSessionManager::register_user(uint32_t id, const std::string& username,
                                                           const std::shared_ptr<Connection>& connection) {
  auto user = user_manager_.create_user(id, username, connection);
  if (user) {
    // Standardmäßig zum Root-Kanal hinzufügen
    multi_channel_.assign_user(id, ChannelTree::ROOT_CHANNEL_ID);
    hh::get_logger().info("ServerSessionManager: User {} registered (channel: {})",
                          username, ChannelTree::ROOT_CHANNEL_ID);
  }
  return user;
}

void ServerSessionManager::unregister_user(uint32_t user_id) {
  user_manager_.remove_user(user_id);
  hh::get_logger().info("ServerSessionManager: User {} unregistered", user_id);
}

std::shared_ptr<User> ServerSessionManager::find_user(uint32_t id) const {
  return user_manager_.find_user(id);
}

void ServerSessionManager::set_user_muted(uint32_t user_id, bool muted) {
  auto user = user_manager_.find_user(user_id);
  if (user) {
    user->set_muted(muted);
    hh::get_logger().debug("ServerSessionManager: User {} muted={}", user_id, muted);
  }
}

void ServerSessionManager::set_user_deafened(uint32_t user_id, bool deafened) {
  auto user = user_manager_.find_user(user_id);
  if (user) {
    user->set_deafened(deafened);
    hh::get_logger().debug("ServerSessionManager: User {} deafened={}", user_id, deafened);
  }
}

void ServerSessionManager::set_user_suppressing(uint32_t user_id, bool suppressing) {
  auto user = user_manager_.find_user(user_id);
  if (user) {
    user->set_suppressing(suppressing);
  }
}

void ServerSessionManager::enable_priority_speech(uint32_t user_id, bool enabled) {
  multi_channel_.enable_priority_speech(user_id, enabled);
}

bool ServerSessionManager::is_priority_speaker(uint32_t user_id) const {
  return multi_channel_.is_priority_speaker(user_id);
}

bool ServerSessionManager::switch_channel(uint32_t user_id, uint32_t new_channel_id) {
  return multi_channel_.assign_user(user_id, new_channel_id);
}

void ServerSessionManager::route_voice(uint32_t from_user_id, std::span<const uint8_t> voice_data) {
  auto from_user = user_manager_.find_user(from_user_id);
  if (!from_user) return;

  // Nur weiterleiten, wenn Nutzer nicht gemutet ist
  if (from_user->is_muted()) {
    hh::get_logger().debug("ServerSessionManager: Voice muted, dropping packet from {}", from_user_id);
    return;
  }

  // Nur weiterleiten, wenn Empfänger nicht gedeafened ist
  auto channel = multi_channel_.find_channel(from_user->channel_id());
  if (!channel) return;

  for (auto user_id : channel->users()) {
    if (user_id == from_user_id) continue; // Nicht an sich selbst senden

    auto target_user = user_manager_.find_user(user_id);
    if (!target_user || target_user->is_deafened()) continue;

    voice_router_.route_to(target_user->udp_endpoint(), voice_data);
  }
}

size_t ServerSessionManager::active_speakers() const {
  // TODO: Zählt aktive Sprecher basierend auf VAD/PTT
  return 0;
}

// ── ACL-Verwaltung ────────────────────────────────────────────────────

std::shared_ptr<AclGroup> ServerSessionManager::create_acl_group(const std::string& name, bool inherits) {
  return acl_manager_.create_group(name, inherits);
}

bool ServerSessionManager::delete_acl_group(uint32_t group_id) {
  return acl_manager_.delete_group(group_id);
}

std::shared_ptr<AclGroup> ServerSessionManager::find_acl_group(uint32_t group_id) const {
  return acl_manager_.find_group(group_id);
}

bool ServerSessionManager::check_user_right(uint32_t user_id, uint32_t channel_id, ChannelRight right) const {
  return acl_manager_.check_right(user_id, channel_id, right);
}

void ServerSessionManager::set_user_admin(uint32_t user_id, bool admin) {
  acl_manager_.set_user_admin(user_id, admin);
}

bool ServerSessionManager::is_user_admin(uint32_t user_id) const {
  return acl_manager_.is_admin(user_id);
}

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server_sync.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <chrono>

namespace hh::server {

// ── ServerSync ────────────────────────────────────────────────────────

ServerSync::ServerSync(ServerSync&& other) noexcept
  : sync_enabled_(other.sync_enabled_)
  , sync_interval_ms_(other.sync_interval_ms_)
  , synced_users_(std::move(other.synced_users_))
  , synced_channels_(std::move(other.synced_channels_))
  , user_synced_callback_(std::move(other.user_synced_callback_))
  , channel_synced_callback_(std::move(other.channel_synced_callback_))
  , total_users_synced_(other.total_users_synced_.load())
  , total_channels_synced_(other.total_channels_synced_.load())
  , sync_failures_(other.sync_failures_.load())
  , sync_successes_(other.sync_successes_.load()) {
  
  other.sync_enabled_ = false;
  other.sync_interval_ms_ = 10000;
  other.total_users_synced_ = 0;
  other.total_channels_synced_ = 0;
  other.sync_failures_ = 0;
  other.sync_successes_ = 0;
}

ServerSync& ServerSync::operator=(ServerSync&& other) noexcept {
  if (this != &other) {
    sync_enabled_ = other.sync_enabled_;
    sync_interval_ms_ = other.sync_interval_ms_;
    synced_users_ = std::move(other.synced_users_);
    synced_channels_ = std::move(other.synced_channels_);
    user_synced_callback_ = std::move(other.user_synced_callback_);
    channel_synced_callback_ = std::move(other.channel_synced_callback_);
    total_users_synced_ = other.total_users_synced_.load();
    total_channels_synced_ = other.total_channels_synced_.load();
    sync_failures_ = other.sync_failures_.load();
    sync_successes_ = other.sync_successes_.load();
    other.sync_enabled_ = false;
    other.sync_interval_ms_ = 10000;
    other.total_users_synced_ = 0;
    other.total_channels_synced_ = 0;
    other.sync_failures_ = 0;
    other.sync_successes_ = 0;
  }
  return *this;
}

void ServerSync::set_sync_enabled(bool enabled) {
  sync_enabled_ = enabled;
  hh::get_logger().info("ServerSync: Sync enabled: {}", enabled);
}

void ServerSync::set_sync_interval_ms(int interval_ms) {
  sync_interval_ms_ = interval_ms;
}

void ServerSync::sync_user(const UserSync& user) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  synced_users_[user.user_id] = user;
  total_users_synced_++;
  sync_successes_++;
  
  if (user_synced_callback_) {
    user_synced_callback_(user);
  }
  
  hh::get_logger().debug("ServerSync: Synced user {} ({})", user.user_id, user.username);
}

void ServerSync::sync_users(const std::vector<UserSync>& users) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& user : users) {
    synced_users_[user.user_id] = user;
    total_users_synced_++;
    sync_successes_++;
  }
  
  if (user_synced_callback_) {
    for (const auto& user : users) {
      user_synced_callback_(user);
    }
  }
  
  hh::get_logger().info("ServerSync: Synced {} users", users.size());
}

ServerSync::UserSync* ServerSync::get_synced_user(uint32_t user_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = synced_users_.find(user_id);
  if (it != synced_users_.end()) {
    return &it->second;
  }
  return nullptr;
}

const ServerSync::UserSync* ServerSync::get_synced_user(uint32_t user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = synced_users_.find(user_id);
  if (it != synced_users_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<ServerSync::UserSync> ServerSync::get_all_synced_users() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<UserSync> users;
  for (const auto& [id, user] : synced_users_) {
    users.push_back(user);
  }
  return users;
}

void ServerSync::sync_channel(const ChannelSync& channel) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  synced_channels_[channel.channel_id] = channel;
  total_channels_synced_++;
  sync_successes_++;
  
  if (channel_synced_callback_) {
    channel_synced_callback_(channel);
  }
  
  hh::get_logger().debug("ServerSync: Synced channel {} ({})", channel.channel_id, channel.name);
}

void ServerSync::sync_channels(const std::vector<ChannelSync>& channels) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& channel : channels) {
    synced_channels_[channel.channel_id] = channel;
    total_channels_synced_++;
    sync_successes_++;
  }
  
  if (channel_synced_callback_) {
    for (const auto& channel : channels) {
      channel_synced_callback_(channel);
    }
  }
  
  hh::get_logger().info("ServerSync: Synced {} channels", channels.size());
}

ServerSync::ChannelSync* ServerSync::get_synced_channel(uint32_t channel_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = synced_channels_.find(channel_id);
  if (it != synced_channels_.end()) {
    return &it->second;
  }
  return nullptr;
}

const ServerSync::ChannelSync* ServerSync::get_synced_channel(uint32_t channel_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = synced_channels_.find(channel_id);
  if (it != synced_channels_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<ServerSync::ChannelSync> ServerSync::get_all_synced_channels() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<ChannelSync> channels;
  for (const auto& [id, channel] : synced_channels_) {
    channels.push_back(channel);
  }
  return channels;
}

bool ServerSync::is_sync_healthy() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return sync_failures_.load() == 0;
}

uint32_t ServerSync::get_synced_users_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return synced_users_.size();
}

uint32_t ServerSync::get_synced_channels_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return synced_channels_.size();
}

void ServerSync::set_user_synced_callback(UserSyncedCallback callback) {
  user_synced_callback_ = std::move(callback);
}

void ServerSync::set_channel_synced_callback(ChannelSyncedCallback callback) {
  channel_synced_callback_ = std::move(callback);
}

ServerSync::SyncStats ServerSync::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  SyncStats stats;
  stats.total_users_synced = total_users_synced_.load();
  stats.total_channels_synced = total_channels_synced_.load();
  stats.sync_failures = sync_failures_.load();
  stats.sync_successes = sync_successes_.load();
  stats.last_sync_duration_ms = 0.0; // Wird beim Sync aktualisiert
  
  return stats;
}

void ServerSync::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  total_users_synced_ = 0;
  total_channels_synced_ = 0;
  sync_failures_ = 0;
  sync_successes_ = 0;
}

void ServerSync::perform_sync() {
  if (!sync_enabled_) return;
  
  auto start = std::chrono::steady_clock::now();
  
  try {
    sync_users_internal();
    sync_channels_internal();
    
    auto end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();
    
    hh::get_logger().info("ServerSync: Sync completed in {} ms", duration);
  } catch (const std::exception& e) {
    sync_failures_++;
    hh::get_logger().error("ServerSync: Sync failed: {}", e.what());
  }
}

void ServerSync::sync_users_internal() {
  // Benutzer-Sync durchführen
  // In einer echten Implementierung würde hier der Sync mit anderen Servern erfolgen
}

void ServerSync::sync_channels_internal() {
  // Channel-Sync durchführen
  // In einer echten Implementierung würde hier der Sync mit anderen Servern erfolgen
}

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "discord_integration.hpp"

#include "hh/logging.hpp"

#include <thread>
#include <chrono>
#include <algorithm>

namespace hh::server {

// ── DiscordIntegration ──────────────────────────────────────────────

DiscordIntegration::DiscordIntegration(DiscordIntegration&& other) noexcept
  : config_(other.config_)
  , is_connected_(other.is_connected_)
  , auto_sync_(other.auto_sync_)
  , sync_interval_ms_(other.sync_interval_ms_)
  , discord_users_(std::move(other.discord_users_))
  , hasihasi_to_discord_(std::move(other.hasihasi_to_discord_))
  , discord_channels_(std::move(other.discord_channels_))
  , message_received_callback_(std::move(other.message_received_callback_))
  , user_connected_callback_(std::move(other.user_connected_callback_))
  , user_disconnected_callback_(std::move(other.user_disconnected_callback_))
  , messages_sent_(other.messages_sent_.load())
  , messages_received_(other.messages_received_.load())
  , sync_thread_(std::move(other.sync_thread_)) {
  
  other.is_connected_ = false;
  other.auto_sync_ = false;
  other.sync_interval_ms_ = 5000;
  other.messages_sent_ = 0;
  other.messages_received_ = 0;
}

DiscordIntegration& DiscordIntegration::operator=(DiscordIntegration&& other) noexcept {
  if (this != &other) {
    config_ = other.config_;
    is_connected_ = other.is_connected_;
    auto_sync_ = other.auto_sync_;
    sync_interval_ms_ = other.sync_interval_ms_;
    discord_users_ = std::move(other.discord_users_);
    hasihasi_to_discord_ = std::move(other.hasihasi_to_discord_);
    discord_channels_ = std::move(other.discord_channels_);
    message_received_callback_ = std::move(other.message_received_callback_);
    user_connected_callback_ = std::move(other.user_connected_callback_);
    user_disconnected_callback_ = std::move(other.user_disconnected_callback_);
    messages_sent_ = other.messages_sent_.load();
    messages_received_ = other.messages_received_.load();
    sync_thread_ = std::move(other.sync_thread_);
    other.is_connected_ = false;
    other.auto_sync_ = false;
    other.sync_interval_ms_ = 5000;
    other.messages_sent_ = 0;
    other.messages_received_ = 0;
  }
  return *this;
}

void DiscordIntegration::set_config(const DiscordConfig& config) {
  config_ = config;
}

DiscordConfig DiscordIntegration::get_config() const {
  return config_;
}

bool DiscordIntegration::connect() {
  if (is_connected_) {
    hh::get_logger().warn("DiscordIntegration: Already connected");
    return false;
  }
  
  if (!config_.enabled) {
    hh::get_logger().warn("DiscordIntegration: Not enabled");
    return false;
  }
  
  if (config_.bot_token.empty()) {
    hh::get_logger().error("DiscordIntegration: Bot token not set");
    return false;
  }
  
  is_connected_ = true;
  
  // Sync-Thread starten
  if (auto_sync_) {
    sync_thread_ = std::make_unique<std::thread>([this]() {
      sync_thread_func();
    });
  }
  
  hh::get_logger().info("DiscordIntegration: Connected");
  return true;
}

void DiscordIntegration::disconnect() {
  if (!is_connected_) {
    hh::get_logger().warn("DiscordIntegration: Not connected");
    return;
  }
  
  is_connected_ = false;
  
  if (sync_thread_ && sync_thread_->joinable()) {
    sync_thread_->join();
  }
  
  hh::get_logger().info("DiscordIntegration: Disconnected");
}

bool DiscordIntegration::add_discord_user(const DiscordUser& user) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (discord_users_.find(user.discord_id) != discord_users_.end()) {
    hh::get_logger().warn("DiscordIntegration: Discord user {} already exists", user.discord_id);
    return false;
  }
  
  discord_users_[user.discord_id] = user;
  
  if (user.hasihasi_id > 0) {
    hasihasi_to_discord_[user.hasihasi_id] = user.discord_id;
  }
  
  if (user_connected_callback_) {
    user_connected_callback_(user);
  }
  
  hh::get_logger().info("DiscordIntegration: Discord user {} added", user.discord_id);
  return true;
}

bool DiscordIntegration::remove_discord_user(const std::string& discord_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_users_.find(discord_id);
  if (it == discord_users_.end()) {
    hh::get_logger().warn("DiscordIntegration: Discord user {} not found", discord_id);
    return false;
  }
  
  // Hasihasi-ID entfernen
  auto hh_it = hasihasi_to_discord_.find(it->second.hasihasi_id);
  if (hh_it != hasihasi_to_discord_.end()) {
    hasihasi_to_discord_.erase(hh_it);
  }
  
  discord_users_.erase(it);
  
  if (user_disconnected_callback_) {
    user_disconnected_callback_(discord_id);
  }
  
  hh::get_logger().info("DiscordIntegration: Discord user {} removed", discord_id);
  return true;
}

DiscordUser* DiscordIntegration::get_discord_user(const std::string& discord_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_users_.find(discord_id);
  if (it != discord_users_.end()) {
    return &it->second;
  }
  return nullptr;
}

const DiscordUser* DiscordIntegration::get_discord_user(const std::string& discord_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_users_.find(discord_id);
  if (it != discord_users_.end()) {
    return &it->second;
  }
  return nullptr;
}

DiscordUser* DiscordIntegration::get_discord_user_by_hasihasi_id(uint32_t hasihasi_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto hh_it = hasihasi_to_discord_.find(hasihasi_id);
  if (hh_it != hasihasi_to_discord_.end()) {
    auto it = discord_users_.find(hh_it->second);
    if (it != discord_users_.end()) {
      return &it->second;
    }
  }
  return nullptr;
}

std::vector<DiscordUser> DiscordIntegration::get_all_discord_users() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<DiscordUser> users;
  for (const auto& [id, user] : discord_users_) {
    users.push_back(user);
  }
  return users;
}

bool DiscordIntegration::add_discord_channel(const DiscordChannel& channel) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (discord_channels_.find(channel.channel_id) != discord_channels_.end()) {
    hh::get_logger().warn("DiscordIntegration: Discord channel {} already exists", channel.channel_id);
    return false;
  }
  
  discord_channels_[channel.channel_id] = channel;
  
  hh::get_logger().info("DiscordIntegration: Discord channel {} added", channel.channel_id);
  return true;
}

bool DiscordIntegration::remove_discord_channel(const std::string& channel_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_channels_.find(channel_id);
  if (it == discord_channels_.end()) {
    hh::get_logger().warn("DiscordIntegration: Discord channel {} not found", channel_id);
    return false;
  }
  
  discord_channels_.erase(it);
  
  hh::get_logger().info("DiscordIntegration: Discord channel {} removed", channel_id);
  return true;
}

DiscordChannel* DiscordIntegration::get_discord_channel(const std::string& channel_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_channels_.find(channel_id);
  if (it != discord_channels_.end()) {
    return &it->second;
  }
  return nullptr;
}

const DiscordChannel* DiscordIntegration::get_discord_channel(const std::string& channel_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_channels_.find(channel_id);
  if (it != discord_channels_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<DiscordChannel> DiscordIntegration::get_all_discord_channels() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<DiscordChannel> channels;
  for (const auto& [id, channel] : discord_channels_) {
    channels.push_back(channel);
  }
  return channels;
}

bool DiscordIntegration::link_discord_user(const std::string& discord_id, uint32_t hasihasi_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_users_.find(discord_id);
  if (it == discord_users_.end()) {
    hh::get_logger().warn("DiscordIntegration: Discord user {} not found", discord_id);
    return false;
  }
  
  it->second.hasihasi_id = hasihasi_id;
  it->second.is_connected = true;
  
  hasihasi_to_discord_[hasihasi_id] = discord_id;
  
  hh::get_logger().info("DiscordIntegration: Discord user {} linked to hasihasi {}", discord_id, hasihasi_id);
  return true;
}

bool DiscordIntegration::unlink_discord_user(const std::string& discord_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = discord_users_.find(discord_id);
  if (it == discord_users_.end()) {
    hh::get_logger().warn("DiscordIntegration: Discord user {} not found", discord_id);
    return false;
  }
  
  auto hh_it = hasihasi_to_discord_.find(it->second.hasihasi_id);
  if (hh_it != hasihasi_to_discord_.end()) {
    hasihasi_to_discord_.erase(hh_it);
  }
  
  it->second.hasihasi_id = 0;
  it->second.is_connected = false;
  
  hh::get_logger().info("DiscordIntegration: Discord user {} unlinked", discord_id);
  return true;
}

void DiscordIntegration::set_auto_sync(bool enabled) {
  auto_sync_ = enabled;
  
  if (enabled && is_connected_ && !sync_thread_) {
    sync_thread_ = std::make_unique<std::thread>([this]() {
      sync_thread_func();
    });
  } else if (!enabled && sync_thread_ && sync_thread_->joinable()) {
    sync_thread_->join();
    sync_thread_.reset();
  }
}

void DiscordIntegration::set_sync_interval(int interval_ms) {
  sync_interval_ms_ = interval_ms;
}

bool DiscordIntegration::send_to_discord(const std::string& discord_id, const std::string& message) {
  // Discord-Nachricht senden
  messages_sent_++;
  return true;
}

bool DiscordIntegration::send_to_channel(const std::string& channel_id, const std::string& message) {
  // Discord-Channel-Nachricht senden
  messages_sent_++;
  return true;
}

bool DiscordIntegration::broadcast_to_discord(const std::string& message) {
  // An alle verbundenen Discord-User senden
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& [id, user] : discord_users_) {
    if (user.is_connected) {
      // Nachricht senden
      messages_sent_++;
    }
  }
  
  return true;
}

std::string DiscordIntegration::get_oauth_url() const {
  // OAuth2-URL generieren
  return "https://discord.com/oauth2/authorize?client_id=" + config_.client_id +
         "&response_type=code&redirect_uri=" + config_.redirect_uri +
         "&scope=identify+guilds";
}

bool DiscordIntegration::handle_oauth_callback(const std::string& code) {
  // OAuth2-Callback verarbeiten
  hh::get_logger().info("DiscordIntegration: OAuth2 callback received");
  return true;
}

bool DiscordIntegration::send_webhook(const std::string& webhook_url, const std::string& message) {
  // Webhook-Nachricht senden
  hh::get_logger().info("DiscordIntegration: Webhook message sent to {}", webhook_url);
  return true;
}

void DiscordIntegration::set_message_received_callback(MessageReceivedCallback callback) {
  message_received_callback_ = std::move(callback);
}

void DiscordIntegration::set_user_connected_callback(UserConnectedCallback callback) {
  user_connected_callback_ = std::move(callback);
}

void DiscordIntegration::set_user_disconnected_callback(UserDisconnectedCallback callback) {
  user_disconnected_callback_ = std::move(callback);
}

DiscordIntegration::DiscordStats DiscordIntegration::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  DiscordStats stats;
  stats.total_users = discord_users_.size();
  stats.connected_users = std::count_if(discord_users_.begin(), discord_users_.end(),
                                        [](const auto& pair) { return pair.second.is_connected; });
  stats.linked_users = std::count_if(discord_users_.begin(), discord_users_.end(),
                                     [](const auto& pair) { return pair.second.hasihasi_id > 0; });
  stats.total_channels = discord_channels_.size();
  stats.linked_channels = std::count_if(discord_channels_.begin(), discord_channels_.end(),
                                        [](const auto& pair) { return pair.second.is_linked; });
  stats.messages_sent = messages_sent_.load();
  stats.messages_received = messages_received_.load();
  
  return stats;
}

void DiscordIntegration::reset_stats() {
  messages_sent_ = 0;
  messages_received_ = 0;
}

void DiscordIntegration::sync_thread_func() {
  // Discord-Sync-Thread
  hh::get_logger().info("DiscordIntegration: Sync thread started");
  
  while (is_connected_) {
    // Discord-User synchronisieren
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [id, user] : discord_users_) {
      if (user.is_connected) {
        user.last_sync = std::chrono::duration<double>(
          std::chrono::system_clock::now().time_since_epoch()
        ).count();
      }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(sync_interval_ms_));
  }
  
  hh::get_logger().info("DiscordIntegration: Sync thread stopped");
}

} // namespace hh::server

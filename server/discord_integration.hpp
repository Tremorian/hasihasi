// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace hh::server {

// Discord-Integration für HasiHasi
class DiscordIntegration {
public:
  struct DiscordConfig {
    std::string bot_token;
    std::string client_id;
    std::string redirect_uri;
    std::string webhook_url;
    bool enabled = false;
    bool auto_sync = false;
    int sync_interval_ms = 5000;
  };

  struct DiscordUser {
    std::string discord_id;
    std::string username;
    std::string discriminator;
    std::string avatar_url;
    uint32_t hasihasi_id = 0;
    bool is_connected = false;
    double last_sync = 0.0;
  };

  struct DiscordChannel {
    std::string channel_id;
    std::string name;
    std::string type; // text, voice, announcement
    uint32_t hasihasi_channel_id = 0;
    bool is_linked = false;
  };

  DiscordIntegration() = default;
  ~DiscordIntegration() = default;

  // Nicht kopierbar, bewegbar
  DiscordIntegration(const DiscordIntegration&) = delete;
  DiscordIntegration& operator=(const DiscordIntegration&) = delete;
  DiscordIntegration(DiscordIntegration&&) noexcept;
  DiscordIntegration& operator=(DiscordIntegration&&) noexcept;

  // Konfiguration
  void set_config(const DiscordConfig& config);
  DiscordConfig get_config() const;

  // Verbindung
  bool connect();
  void disconnect();
  bool is_connected() const { return is_connected_; }

  // Discord-User-Verwaltung
  bool add_discord_user(const DiscordUser& user);
  bool remove_discord_user(const std::string& discord_id);
  DiscordUser* get_discord_user(const std::string& discord_id);
  const DiscordUser* get_discord_user(const std::string& discord_id) const;
  DiscordUser* get_discord_user_by_hasihasi_id(uint32_t hasihasi_id);
  std::vector<DiscordUser> get_all_discord_users() const;

  // Discord-Channel-Verwaltung
  bool add_discord_channel(const DiscordChannel& channel);
  bool remove_discord_channel(const std::string& channel_id);
  DiscordChannel* get_discord_channel(const std::string& channel_id);
  const DiscordChannel* get_discord_channel(const std::string& channel_id) const;
  std::vector<DiscordChannel> get_all_discord_channels() const;

  // Linking
  bool link_discord_user(const std::string& discord_id, uint32_t hasihasi_id);
  bool unlink_discord_user(const std::string& discord_id);

  // Synchronisation
  void set_auto_sync(bool enabled);
  bool is_auto_sync_enabled() const { return auto_sync_; }
  void set_sync_interval(int interval_ms);
  int get_sync_interval() const { return sync_interval_ms_; }

  // Nachrichten
  bool send_to_discord(const std::string& discord_id, const std::string& message);
  bool send_to_channel(const std::string& channel_id, const std::string& message);
  bool broadcast_to_discord(const std::string& message);

  // OAuth2
  std::string get_oauth_url() const;
  bool handle_oauth_callback(const std::string& code);

  // Webhook
  bool send_webhook(const std::string& webhook_url, const std::string& message);

  // Callbacks
  using MessageReceivedCallback = std::function<void(const std::string& sender_id, const std::string& message)>;
  using UserConnectedCallback = std::function<void(const DiscordUser& user)>;
  using UserDisconnectedCallback = std::function<void(const std::string& discord_id)>;
  
  void set_message_received_callback(MessageReceivedCallback callback);
  void set_user_connected_callback(UserConnectedCallback callback);
  void set_user_disconnected_callback(UserDisconnectedCallback callback);

  // Statistiken
  struct DiscordStats {
    uint32_t total_users = 0;
    uint32_t connected_users = 0;
    uint32_t linked_users = 0;
    uint32_t total_channels = 0;
    uint32_t linked_channels = 0;
    uint32_t messages_sent = 0;
    uint32_t messages_received = 0;
  };

  DiscordStats get_stats() const;
  void reset_stats();

private:
  // Sync-Thread
  void sync_thread_func();
  
  // Einstellungen
  DiscordConfig config_;
  bool is_connected_ = false;
  bool auto_sync_ = false;
  int sync_interval_ms_ = 5000;
  
  // Discord-User
  mutable std::mutex mutex_;
  std::unordered_map<std::string, DiscordUser> discord_users_;
  std::unordered_map<uint32_t, std::string> hasihasi_to_discord_;
  
  // Discord-Channel
  std::unordered_map<std::string, DiscordChannel> discord_channels_;
  
  // Callbacks
  MessageReceivedCallback message_received_callback_;
  UserConnectedCallback user_connected_callback_;
  UserDisconnectedCallback user_disconnected_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> messages_sent_{0};
  mutable std::atomic<uint32_t> messages_received_{0};
  
  // Thread
  std::unique_ptr<std::thread> sync_thread_;
};

} // namespace hh::server

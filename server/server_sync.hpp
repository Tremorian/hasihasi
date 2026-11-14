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

// Server-Sync: Benutzer/Channels zwischen Servern synchronisieren
class ServerSync {
public:
  struct UserSync {
    uint32_t user_id;
    std::string username;
    std::string email;
    std::vector<uint32_t> channel_ids;
    bool is_admin = false;
    bool is_online = false;
  };

  struct ChannelSync {
    uint32_t channel_id;
    std::string name;
    uint32_t parent_id;
    uint32_t max_users;
    bool is_temporary = false;
    std::string description;
  };

  ServerSync() = default;
  ~ServerSync() = default;

  // Nicht kopierbar, bewegbar
  ServerSync(const ServerSync&) = delete;
  ServerSync& operator=(const ServerSync&) = delete;
  ServerSync(ServerSync&&) noexcept;
  ServerSync& operator=(ServerSync&&) noexcept;

  // Sync konfigurieren
  void set_sync_enabled(bool enabled);
  bool is_sync_enabled() const { return sync_enabled_; }

  void set_sync_interval_ms(int interval_ms);
  int get_sync_interval_ms() const { return sync_interval_ms_; }

  // Benutzer synchronisieren
  void sync_user(const UserSync& user);
  void sync_users(const std::vector<UserSync>& users);
  UserSync* get_synced_user(uint32_t user_id);
  const UserSync* get_synced_user(uint32_t user_id) const;
  std::vector<UserSync> get_all_synced_users() const;

  // Channels synchronisieren
  void sync_channel(const ChannelSync& channel);
  void sync_channels(const std::vector<ChannelSync>& channels);
  ChannelSync* get_synced_channel(uint32_t channel_id);
  const ChannelSync* get_synced_channel(uint32_t channel_id) const;
  std::vector<ChannelSync> get_all_synced_channels() const;

  // Sync-Status
  bool is_sync_healthy() const;
  uint32_t get_synced_users_count() const;
  uint32_t get_synced_channels_count() const;

  // Callbacks
  using UserSyncedCallback = std::function<void(const UserSync& user)>;
  using ChannelSyncedCallback = std::function<void(const ChannelSync& channel)>;
  
  void set_user_synced_callback(UserSyncedCallback callback);
  void set_channel_synced_callback(ChannelSyncedCallback callback);

  // Statistiken
  struct SyncStats {
    uint32_t total_users_synced = 0;
    uint32_t total_channels_synced = 0;
    uint32_t sync_failures = 0;
    uint32_t sync_successes = 0;
    double last_sync_duration_ms = 0.0;
  };

  SyncStats get_stats() const;
  void reset_stats();

private:
  // Sync-Schritt ausführen
  void perform_sync();
  
  // Benutzer-Sync durchführen
  void sync_users_internal();
  
  // Channel-Sync durchführen
  void sync_channels_internal();
  
  // Einstellungen
  bool sync_enabled_ = false;
  int sync_interval_ms_ = 10000; // 10 Sekunden
  
  // Sync-Daten
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, UserSync> synced_users_;
  std::unordered_map<uint32_t, ChannelSync> synced_channels_;
  
  // Callbacks
  UserSyncedCallback user_synced_callback_;
  ChannelSyncedCallback channel_synced_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> total_users_synced_{0};
  mutable std::atomic<uint32_t> total_channels_synced_{0};
  mutable std::atomic<uint32_t> sync_failures_{0};
  mutable std::atomic<uint32_t> sync_successes_{0};
};

} // namespace hh::server

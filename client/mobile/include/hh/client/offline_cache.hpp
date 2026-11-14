// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QDateTime>
#include <vector>
#include <map>
#include <mutex>

namespace hh::client {

// Offline-Mode: Lokaler Cache für Nachrichten und Kanalstruktur
class OfflineCache : public QObject {
  Q_OBJECT

public:
  explicit OfflineCache(QObject* parent = nullptr);
  ~OfflineCache() override = default;

  // Nicht kopierbar, bewegbar
  OfflineCache(const OfflineCache&) = delete;
  OfflineCache& operator=(const OfflineCache&) = delete;
  OfflineCache(OfflineCache&&) noexcept;
  OfflineCache& operator=(OfflineCache&&) noexcept;

  // Kanal-Cache
  struct Channel {
    uint32_t id;
    QString name;
    uint32_t parent_id;
    uint32_t max_users;
    bool temporary;
    QString description;
  };

  bool save_channel(const Channel& channel);
  bool delete_channel(uint32_t channel_id);
  Channel* find_channel(uint32_t channel_id);
  const Channel* find_channel(uint32_t channel_id) const;
  std::vector<Channel> all_channels() const;
  size_t channel_count() const;

  // Nutzer-Cache
  struct User {
    uint32_t id;
    QString username;
    uint32_t channel_id;
    bool muted;
    bool deafened;
    bool suppressing;
  };

  bool save_user(const User& user);
  bool delete_user(uint32_t user_id);
  User* find_user(uint32_t user_id);
  const User* find_user(uint32_t user_id) const;
  std::vector<User> all_users() const;
  size_t user_count() const;

  // Nachrichten-Cache
  struct Message {
    uint32_t id;
    uint32_t sender_id;
    QString sender_name;
    QString content;
    bool is_private;
    uint32_t target_id;
    QDateTime timestamp;
  };

  bool save_message(const Message& message);
  bool delete_message(uint32_t message_id);
  std::vector<Message> get_channel_messages(uint32_t channel_id, int limit = 50) const;
  std::vector<Message> get_private_messages(uint32_t user_id, int limit = 50) const;
  size_t message_count() const;

  // Cache-Verwaltung
  void clear_all();
  void clear_channels();
  void clear_users();
  void clear_messages();

  // Cache-Größe
  struct CacheStats {
    size_t channel_count;
    size_t user_count;
    size_t message_count;
    size_t total_size_bytes;
  };

  CacheStats get_stats() const;
  size_t total_size_bytes() const;

  // Konfiguration
  void set_max_messages(int max_messages);
  void set_max_cache_size_mb(int max_size_mb);
  int max_messages() const;
  int max_cache_size_mb() const;

  // Offline-Status
  enum class Status {
    OFFLINE,
    SYNCING,
    ONLINE
  };
  Q_ENUM(Status)

  Status status() const;
  void set_status(Status status);

  // Sync-Status
  struct SyncStatus {
    bool channels_synced;
    bool users_synced;
    bool messages_synced;
    QDateTime last_sync;
  };

  SyncStatus get_sync_status() const;
  void set_sync_status(bool channels, bool users, bool messages);

  // Callbacks
  using CacheCallback = std::function<void(const QString& event, const QVariant& data)>;
  void set_cache_callback(CacheCallback callback);

signals:
  void status_changed(Status status);
  void cache_updated(const QString& type, const QVariant& data);
  void sync_completed();
  void error_occurred(const QString& error);

private slots:
  void on_cache_full();

private:
  mutable std::mutex mutex_;
  
  std::map<uint32_t, Channel> channels_;
  std::map<uint32_t, User> users_;
  std::vector<Message> messages_;
  
  int max_messages_ = 1000;
  int max_cache_size_mb_ = 100;
  
  Status status_ = Status::OFFLINE;
  SyncStatus sync_status_ = {false, false, false, QDateTime()};
  
  CacheCallback cache_callback_;
};

} // namespace hh::client

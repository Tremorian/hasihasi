// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "offline_cache.hpp"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

namespace hh::client {

OfflineCache::OfflineCache(QObject* parent)
  : QObject(parent) {
  
  // Cache aus Datei laden
  load_from_disk();
}

OfflineCache::OfflineCache(OfflineCache&& other) noexcept
  : QObject(other.parent())
  , channels_(std::move(other.channels_))
  , users_(std::move(other.users_))
  , messages_(std::move(other.messages_))
  , max_messages_(other.max_messages_)
  , max_cache_size_mb_(other.max_cache_size_mb_)
  , status_(other.status_)
  , sync_status_(other.sync_status_)
  , cache_callback_(std::move(other.cache_callback_)) {
  
  other.max_messages_ = 1000;
  other.max_cache_size_mb_ = 100;
  other.status_ = Status::OFFLINE;
  other.sync_status_ = SyncStatus{false, false, false, QDateTime()};
}

OfflineCache& OfflineCache::operator=(OfflineCache&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    channels_ = std::move(other.channels_);
    users_ = std::move(other.users_);
    messages_ = std::move(other.messages_);
    max_messages_ = other.max_messages_;
    max_cache_size_mb_ = other.max_cache_size_mb_;
    status_ = other.status_;
    sync_status_ = other.sync_status_;
    cache_callback_ = std::move(other.cache_callback_);
    other.max_messages_ = 1000;
    other.max_cache_size_mb_ = 100;
    other.status_ = Status::OFFLINE;
    other.sync_status_ = SyncStatus{false, false, false, QDateTime()};
  }
  return *this;
}

// ── Kanal-Cache ──────────────────────────────────────────────────────

bool OfflineCache::save_channel(const Channel& channel) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  channels_[channel.id] = channel;
  emit cache_updated("channel", QVariant::fromValue(channel));
  
  save_to_disk();
  return true;
}

bool OfflineCache::delete_channel(uint32_t channel_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = channels_.find(channel_id);
  if (it != channels_.end()) {
    channels_.erase(it);
    emit cache_updated("channel_deleted", QVariant::fromValue(channel_id));
    save_to_disk();
    return true;
  }
  return false;
}

OfflineCache* OfflineCache::find_channel(uint32_t channel_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = channels_.find(channel_id);
  if (it != channels_.end()) {
    return &it->second;
  }
  return nullptr;
}

const OfflineCache* OfflineCache::find_channel(uint32_t channel_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = channels_.find(channel_id);
  if (it != channels_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<OfflineCache::Channel> OfflineCache::all_channels() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Channel> result;
  for (const auto& pair : channels_) {
    result.push_back(pair.second);
  }
  return result;
}

size_t OfflineCache::channel_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return channels_.size();
}

// ── Nutzer-Cache ─────────────────────────────────────────────────────

bool OfflineCache::save_user(const User& user) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  users_[user.id] = user;
  emit cache_updated("user", QVariant::fromValue(user));
  
  save_to_disk();
  return true;
}

bool OfflineCache::delete_user(uint32_t user_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    users_.erase(it);
    emit cache_updated("user_deleted", QVariant::fromValue(user_id));
    save_to_disk();
    return true;
  }
  return false;
}

OfflineCache* OfflineCache::find_user(uint32_t user_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    return &it->second;
  }
  return nullptr;
}

const OfflineCache* OfflineCache::find_user(uint32_t user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<OfflineCache::User> OfflineCache::all_users() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<User> result;
  for (const auto& pair : users_) {
    result.push_back(pair.second);
  }
  return result;
}

size_t OfflineCache::user_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return users_.size();
}

// ── Nachrichten-Cache ────────────────────────────────────────────────

bool OfflineCache::save_message(const Message& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  messages_.push_back(message);
  
  // Maximalgröße prüfen
  if (static_cast<int>(messages_.size()) > max_messages_) {
    on_cache_full();
  }
  
  emit cache_updated("message", QVariant::fromValue(message));
  save_to_disk();
  return true;
}

bool OfflineCache::delete_message(uint32_t message_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = std::find_if(messages_.begin(), messages_.end(),
                          [message_id](const Message& msg) { return msg.id == message_id; });
  if (it != messages_.end()) {
    messages_.erase(it);
    emit cache_updated("message_deleted", QVariant::fromValue(message_id));
    save_to_disk();
    return true;
  }
  return false;
}

std::vector<OfflineCache::Message> OfflineCache::get_channel_messages(uint32_t channel_id, int limit) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Message> result;
  int count = 0;
  
  for (auto it = messages_.rbegin(); it != messages_.rend() && count < limit; ++it) {
    if (!it->is_private && it->target_id == channel_id) {
      result.push_back(*it);
      count++;
    }
  }
  
  return result;
}

std::vector<OfflineCache::Message> OfflineCache::get_private_messages(uint32_t user_id, int limit) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Message> result;
  int count = 0;
  
  for (auto it = messages_.rbegin(); it != messages_.rend() && count < limit; ++it) {
    if (it->is_private && (it->sender_id == user_id || it->target_id == user_id)) {
      result.push_back(*it);
      count++;
    }
  }
  
  return result;
}

size_t OfflineCache::message_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return messages_.size();
}

// ── Cache-Verwaltung ─────────────────────────────────────────────────

void OfflineCache::clear_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  channels_.clear();
  users_.clear();
  messages_.clear();
  
  emit cache_updated("clear_all", QVariant());
  save_to_disk();
}

void OfflineCache::clear_channels() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  channels_.clear();
  emit cache_updated("clear_channels", QVariant());
  save_to_disk();
}

void OfflineCache::clear_users() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  users_.clear();
  emit cache_updated("clear_users", QVariant());
  save_to_disk();
}

void OfflineCache::clear_messages() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  messages_.clear();
  emit cache_updated("clear_messages", QVariant());
  save_to_disk();
}

// ── Cache-Größe ──────────────────────────────────────────────────────

OfflineCache::CacheStats OfflineCache::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  CacheStats stats;
  stats.channel_count = channels_.size();
  stats.user_count = users_.size();
  stats.message_count = messages_.size();
  stats.total_size_bytes = total_size_bytes();
  
  return stats;
}

size_t OfflineCache::total_size_bytes() const {
  // Geschätzte Größe (einfache Berechnung)
  size_t size = 0;
  size += channels_.size() * 256; // ~256 Byte pro Kanal
  size += users_.size() * 128;    // ~128 Byte pro Nutzer
  size += messages_.size() * 512; // ~512 Byte pro Nachricht
  return size;
}

// ── Konfiguration ────────────────────────────────────────────────────

void OfflineCache::set_max_messages(int max_messages) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_messages_ = max_messages;
}

void OfflineCache::set_max_cache_size_mb(int max_size_mb) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_cache_size_mb_ = max_size_mb;
}

int OfflineCache::max_messages() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return max_messages_;
}

int OfflineCache::max_cache_size_mb() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return max_cache_size_mb_;
}

// ── Offline-Status ───────────────────────────────────────────────────

OfflineCache::Status OfflineCache::status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return status_;
}

void OfflineCache::set_status(Status status) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ != status) {
    status_ = status;
    emit status_changed(status);
  }
}

// ── Sync-Status ──────────────────────────────────────────────────────

OfflineCache::SyncStatus OfflineCache::get_sync_status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return sync_status_;
}

void OfflineCache::set_sync_status(bool channels, bool users, bool messages) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  sync_status_.channels_synced = channels;
  sync_status_.users_synced = users;
  sync_status_.messages_synced = messages;
  sync_status_.last_sync = QDateTime::currentDateTime();
  
  if (channels && users && messages) {
    emit sync_completed();
  }
}

// ── Callbacks ────────────────────────────────────────────────────────

void OfflineCache::set_cache_callback(CacheCallback callback) {
  cache_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void OfflineCache::on_cache_full() {
  // Älteste Nachrichten entfernen
  int remove_count = messages_.size() / 4; // 25% entfernen
  if (remove_count > 0) {
    messages_.erase(messages_.begin(), messages_.begin() + remove_count);
    emit cache_updated("cache_full", QVariant::fromValue(remove_count));
    save_to_disk();
  }
}

// ── Persistenz ───────────────────────────────────────────────────────

void OfflineCache::save_to_disk() const {
  // Cache in JSON-Datei speichern
  QString cache_file = QDir::temp().absoluteFilePath("hasihasi_cache.json");
  
  QJsonObject json;
  
  // Kanäle speichern
  QJsonArray channels_array;
  for (const auto& pair : channels_) {
    QJsonObject channel;
    channel["id"] = pair.second.id;
    channel["name"] = pair.second.name;
    channel["parent_id"] = pair.second.parent_id;
    channel["max_users"] = pair.second.max_users;
    channel["temporary"] = pair.second.temporary;
    channel["description"] = pair.second.description;
    channels_array.append(channel);
  }
  json["channels"] = channels_array;
  
  // Nutzer speichern
  QJsonArray users_array;
  for (const auto& pair : users_) {
    QJsonObject user;
    user["id"] = pair.second.id;
    user["username"] = pair.second.username;
    user["channel_id"] = pair.second.channel_id;
    user["muted"] = pair.second.muted;
    user["deafened"] = pair.second.deafened;
    user["suppressing"] = pair.second.suppressing;
    users_array.append(user);
  }
  json["users"] = users_array;
  
  // Nachrichten speichern
  QJsonArray messages_array;
  for (const auto& msg : messages_) {
    QJsonObject message;
    message["id"] = msg.id;
    message["sender_id"] = msg.sender_id;
    message["sender_name"] = msg.sender_name;
    message["content"] = msg.content;
    message["is_private"] = msg.is_private;
    message["target_id"] = msg.target_id;
    message["timestamp"] = msg.timestamp.toString(Qt::ISODate);
    messages_array.append(message);
  }
  json["messages"] = messages_array;
  
  // Datei schreiben
  QFile file(cache_file);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(QJsonDocument(json).toJson());
    file.close();
  }
}

void OfflineCache::load_from_disk() {
  // Cache aus JSON-Datei laden
  QString cache_file = QDir::temp().absoluteFilePath("hasihasi_cache.json");
  
  QFile file(cache_file);
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }
  
  QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));
  file.close();
  
  QJsonObject json = doc.object();
  
  // Kanäle laden
  QJsonArray channels_array = json["channels"].toArray();
  for (const QJsonValue& value : channels_array) {
    QJsonObject channel = value.toObject();
    Channel ch;
    ch.id = channel["id"].toInt();
    ch.name = channel["name"].toString();
    ch.parent_id = channel["parent_id"].toInt();
    ch.max_users = channel["max_users"].toInt();
    ch.temporary = channel["temporary"].toBool();
    ch.description = channel["description"].toString();
    channels_[ch.id] = ch;
  }
  
  // Nutzer laden
  QJsonArray users_array = json["users"].toArray();
  for (const QJsonValue& value : users_array) {
    QJsonObject user = value.toObject();
    User u;
    u.id = user["id"].toInt();
    u.username = user["username"].toString();
    u.channel_id = user["channel_id"].toInt();
    u.muted = user["muted"].toBool();
    u.deafened = user["deafened"].toBool();
    u.suppressing = user["suppressing"].toBool();
    users_[u.id] = u;
  }
  
  // Nachrichten laden
  QJsonArray messages_array = json["messages"].toArray();
  for (const QJsonValue& value : messages_array) {
    QJsonObject message = value.toObject();
    Message msg;
    msg.id = message["id"].toInt();
    msg.sender_id = message["sender_id"].toInt();
    msg.sender_name = message["sender_name"].toString();
    msg.content = message["content"].toString();
    msg.is_private = message["is_private"].toBool();
    msg.target_id = message["target_id"].toInt();
    msg.timestamp = QDateTime::fromString(message["timestamp"].toString(), Qt::ISODate);
    messages_.push_back(msg);
  }
}

} // namespace hh::client

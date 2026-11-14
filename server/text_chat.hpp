// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace hh::server {

// Repräsentiert eine Textnachricht
class TextMessage {
public:
  TextMessage(uint32_t id, uint32_t sender_id, const std::string& sender_name,
              const std::string& content, bool is_private, uint32_t target_id,
              uint32_t channel_id);

  // Eigenschaften
  uint32_t id() const { return id_; }
  uint32_t sender_id() const { return sender_id_; }
  const std::string& sender_name() const { return sender_name_; }
  const std::string& content() const { return content_; }
  bool is_private() const { return is_private_; }
  uint32_t target_id() const { return target_id_; }
  uint32_t channel_id() const { return channel_id_; }

private:
  uint32_t id_;
  uint32_t sender_id_;
  std::string sender_name_;
  std::string content_;
  bool is_private_;
  uint32_t target_id_;
  uint32_t channel_id_;
};

// TextChat-Verwaltung für Kanal- und Privatenachrichten
class TextChat {
public:
  TextChat() = default;

  // Nachricht senden
  std::shared_ptr<TextMessage> send_message(uint32_t sender_id, const std::string& sender_name,
                                             const std::string& content,
                                             bool is_private, uint32_t target_id,
                                             uint32_t channel_id);

  // Nachrichten laden
  std::vector<std::shared_ptr<TextMessage>> get_channel_messages(uint32_t channel_id, int limit = 50) const;
  std::vector<std::shared_ptr<TextMessage>> get_private_messages(uint32_t user_id, int limit = 50) const;
  std::vector<std::shared_ptr<TextMessage>> get_all_messages(int limit = 100) const;

  // Nachricht löschen
  bool delete_message(uint32_t message_id);

  // Statistik
  size_t message_count() const { return messages_.size(); }
  size_t channel_message_count(uint32_t channel_id) const;
  size_t private_message_count(uint32_t user_id) const;

private:
  std::map<uint32_t, std::shared_ptr<TextMessage>> messages_;
  uint32_t next_id_ = 1;
};

} // namespace hh::server

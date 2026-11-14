// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/text_chat.hpp"

#include "hh/logging.hpp"

namespace hh::server {

// ── TextMessage ───────────────────────────────────────────────────────

TextMessage::TextMessage(uint32_t id, uint32_t sender_id, const std::string& sender_name,
                         const std::string& content, bool is_private, uint32_t target_id,
                         uint32_t channel_id)
  : id_(id)
  , sender_id_(sender_id)
  , sender_name_(sender_name)
  , content_(content)
  , is_private_(is_private)
  , target_id_(target_id)
  , channel_id_(channel_id) {
}

// ── TextChat ──────────────────────────────────────────────────────────

std::shared_ptr<TextMessage> TextChat::send_message(uint32_t sender_id, const std::string& sender_name,
                                                     const std::string& content,
                                                     bool is_private, uint32_t target_id,
                                                     uint32_t channel_id) {
  auto message = std::make_shared<TextMessage>(next_id_, sender_id, sender_name, content,
                                                is_private, target_id, channel_id);
  messages_[next_id_] = message;
  next_id_++;

  hh::get_logger().info("TextChat: Message {} sent by {} (private: {}, target: {}, channel: {})",
                        next_id_ - 1, sender_name, is_private, target_id, channel_id);
  return message;
}

std::vector<std::shared_ptr<TextMessage>> TextChat::get_channel_messages(uint32_t channel_id, int limit) const {
  std::vector<std::shared_ptr<TextMessage>> result;

  for (const auto& [id, message] : messages_) {
    if (message->channel_id() == channel_id && !message->is_private()) {
      result.push_back(message);
      if (static_cast<int>(result.size()) >= limit) {
        break;
      }
    }
  }

  return result;
}

std::vector<std::shared_ptr<TextMessage>> TextChat::get_private_messages(uint32_t user_id, int limit) const {
  std::vector<std::shared_ptr<TextMessage>> result;

  for (const auto& [id, message] : messages_) {
    if (message->is_private() && (message->target_id() == user_id || message->sender_id() == user_id)) {
      result.push_back(message);
      if (static_cast<int>(result.size()) >= limit) {
        break;
      }
    }
  }

  return result;
}

std::vector<std::shared_ptr<TextMessage>> TextChat::get_all_messages(int limit) const {
  std::vector<std::shared_ptr<TextMessage>> result;

  for (const auto& [id, message] : messages_) {
    result.push_back(message);
    if (static_cast<int>(result.size()) >= limit) {
      break;
    }
  }

  return result;
}

bool TextChat::delete_message(uint32_t message_id) {
  auto it = messages_.find(message_id);
  if (it == messages_.end()) {
    return false;
  }

  messages_.erase(it);
  hh::get_logger().info("TextChat: Message {} deleted", message_id);
  return true;
}

size_t TextChat::channel_message_count(uint32_t channel_id) const {
  size_t count = 0;
  for (const auto& [id, message] : messages_) {
    if (message->channel_id() == channel_id && !message->is_private()) {
      count++;
    }
  }
  return count;
}

size_t TextChat::private_message_count(uint32_t user_id) const {
  size_t count = 0;
  for (const auto& [id, message] : messages_) {
    if (message->is_private() && (message->target_id() == user_id || message->sender_id() == user_id)) {
      count++;
    }
  }
  return count;
}

} // namespace hh::server

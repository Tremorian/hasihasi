// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/channel.hpp"

#include "hh/logging.hpp"

namespace hh::server {

// ── Channel ───────────────────────────────────────────────────────────

Channel::Channel(uint32_t id, uint32_t parent_id, const std::string& name,
                 uint32_t max_users, bool temporary, const std::string& description)
  : id_(id)
  , parent_id_(parent_id)
  , name_(name)
  , max_users_(max_users)
  , temporary_(temporary)
  , description_(description) {
}

void Channel::add_user(uint32_t user_id) {
  users_.push_back(user_id);
}

void Channel::remove_user(uint32_t user_id) {
  users_.erase(std::remove(users_.begin(), users_.end(), user_id), users_.end());
}

void Channel::add_child(uint32_t child_id) {
  children_.push_back(child_id);
}

// ── ChannelTree ───────────────────────────────────────────────────────

ChannelTree::ChannelTree() {
  // Root-Kanal erstellen
  auto root = std::make_shared<Channel>(ROOT_CHANNEL_ID, 0, "Root", 0, false, "");
  channels_[ROOT_CHANNEL_ID] = root;
}

std::shared_ptr<Channel> ChannelTree::create_channel(uint32_t parent_id, const std::string& name,
                                                      uint32_t max_users,
                                                      bool temporary,
                                                      const std::string& description) {
  auto parent = find_channel(parent_id);
  if (!parent) {
    hh::get_logger().error("Parent channel {} not found", parent_id);
    return nullptr;
  }

  auto channel = std::make_shared<Channel>(next_id_, parent_id, name, max_users, temporary, description);
  channels_[next_id_] = channel;
  parent->add_child(next_id_);

  hh::get_logger().info("Created channel {} '{}' (parent: {}, max_users: {})",
                        next_id_, name, parent_id, max_users);
  return channel;
}

bool ChannelTree::delete_channel(uint32_t channel_id) {
  auto it = channels_.find(channel_id);
  if (it == channels_.end()) {
    return false;
  }

  auto channel = it->second;

  // Temporäre Kanäle können gelöscht werden
  if (!channel->is_temporary()) {
    hh::get_logger().warn("Cannot delete non-temporary channel {}", channel_id);
    return false;
  }

  // Nutzer entfernen
  for (auto user_id : channel->users()) {
    // Nutzer zum Root-Kanal bewegen (vereinfacht)
    (void)user_id;
  }

  // Unterkanae löschen (rekursiv)
  for (auto child_id : channel->children()) {
    delete_channel(child_id);
  }

  // Vom Parent entfernen
  auto parent = find_channel(channel->parent_id());
  if (parent) {
    parent->remove_user(channel_id); // Vereinfacht
  }

  channels_.erase(it);
  hh::get_logger().info("Deleted channel {}", channel_id);
  return true;
}

std::shared_ptr<Channel> ChannelTree::find_channel(uint32_t channel_id) const {
  auto it = channels_.find(channel_id);
  if (it != channels_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<Channel>> ChannelTree::all_channels() const {
  std::vector<std::shared_ptr<Channel>> result;
  for (const auto& [id, channel] : channels_) {
    result.push_back(channel);
  }
  return result;
}

bool ChannelTree::assign_user_to_channel(uint32_t user_id, uint32_t channel_id) {
  auto channel = find_channel(channel_id);
  if (!channel) {
    return false;
  }

  if (channel->is_full()) {
    hh::get_logger().warn("Channel {} is full", channel_id);
    return false;
  }

  // Vom alten Kanal entfernen (vereinfacht)
  channel->add_user(user_id);
  hh::get_logger().info("User {} assigned to channel {}", user_id, channel_id);
  return true;
}

} // namespace hh::server

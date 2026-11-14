// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace hh::server {

// Repräsentiert einen Kanal im Kanalbaum
class Channel {
public:
  Channel(uint32_t id, uint32_t parent_id, const std::string& name,
          uint32_t max_users, bool temporary, const std::string& description);

  // Grundlegende Eigenschaften
  uint32_t id() const { return id_; }
  uint32_t parent_id() const { return parent_id_; }
  const std::string& name() const { return name_; }
  uint32_t max_users() const { return max_users_; }
  bool is_temporary() const { return temporary_; }
  const std::string& description() const { return description_; }

  // Nutzer-Verwaltung
  void add_user(uint32_t user_id);
  void remove_user(uint32_t user_id);
  std::vector<uint32_t> users() const { return users_; }
  size_t user_count() const { return users_.size(); }
  bool is_full() const { return max_users_ > 0 && users_.size() >= max_users_; }

  // Unterkanae
  void add_child(uint32_t child_id);
  std::vector<uint32_t> children() const { return children_; }

private:
  uint32_t id_;
  uint32_t parent_id_;
  std::string name_;
  uint32_t max_users_;
  bool temporary_;
  std::string description_;
  std::vector<uint32_t> users_;
  std::vector<uint32_t> children_;
};

// Kanalbaum-Verwaltung
class ChannelTree {
public:
  // Root-Kanal erstellen (immer vorhanden)
  ChannelTree();

  // Kanal erstellen
  std::shared_ptr<Channel> create_channel(uint32_t parent_id, const std::string& name,
                                           uint32_t max_users = 0,
                                           bool temporary = false,
                                           const std::string& description = "");

  // Kanal löschen
  bool delete_channel(uint32_t channel_id);

  // Kanal finden
  std::shared_ptr<Channel> find_channel(uint32_t channel_id) const;

  // Alle Kanäle
  std::vector<std::shared_ptr<Channel>> all_channels() const;

  // Nutzer einem Kanal zuweisen
  bool assign_user_to_channel(uint32_t user_id, uint32_t channel_id);

  // Root-Kanal-ID
  static constexpr uint32_t ROOT_CHANNEL_ID = 1;

private:
  std::map<uint32_t, std::shared_ptr<Channel>> channels_;
  uint32_t next_id_ = 2; // 1 ist Root
};

} // namespace hh::server

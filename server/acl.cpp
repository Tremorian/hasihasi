// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/acl.hpp"

#include "hh/logging.hpp"

namespace hh::server {

// ── AclGroup ──────────────────────────────────────────────────────────

AclGroup::AclGroup(uint32_t id, const std::string& name)
  : id_(id)
  , name_(name) {
}

void AclGroup::set_rights(uint32_t channel_id, ChannelRight rights) {
  channel_rights_[channel_id] = rights;
  hh::get_logger().debug("AclGroup {}: Rights for channel {} set to {}", id_, channel_id,
                         static_cast<uint16_t>(rights));
}

ChannelRight AclGroup::get_rights(uint32_t channel_id) const {
  auto it = channel_rights_.find(channel_id);
  if (it != channel_rights_.end()) {
    return it->second;
  }
  return ChannelRight::None;
}

void AclGroup::add_member(uint32_t user_id) {
  members_.push_back(user_id);
  hh::get_logger().debug("AclGroup {}: User {} added", id_, user_id);
}

void AclGroup::remove_member(uint32_t user_id) {
  members_.erase(std::remove(members_.begin(), members_.end(), user_id), members_.end());
  hh::get_logger().debug("AclGroup {}: User {} removed", id_, user_id);
}

bool AclGroup::has_member(uint32_t user_id) const {
  return std::find(members_.begin(), members_.end(), user_id) != members_.end();
}

// ── AclManager ────────────────────────────────────────────────────────

std::shared_ptr<AclGroup> AclManager::create_group(const std::string& name, bool inherits) {
  uint32_t id = static_cast<uint32_t>(groups_.size()) + 1;
  auto group = std::make_shared<AclGroup>(id, name);
  group->set_inherits(inherits);
  groups_[id] = group;
  hh::get_logger().info("AclManager: Created group '{}' (id: {}, inherits: {})",
                        name, id, inherits);
  return group;
}

bool AclManager::delete_group(uint32_t group_id) {
  auto it = groups_.find(group_id);
  if (it == groups_.end()) {
    return false;
  }
  groups_.erase(it);
  hh::get_logger().info("AclManager: Deleted group {}", group_id);
  return true;
}

std::shared_ptr<AclGroup> AclManager::find_group(uint32_t group_id) const {
  auto it = groups_.find(group_id);
  if (it != groups_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<AclGroup> AclManager::find_group_by_name(const std::string& name) const {
  for (const auto& [id, group] : groups_) {
    if (group->name() == name) {
      return group;
    }
  }
  return nullptr;
}

std::vector<std::shared_ptr<AclGroup>> AclManager::all_groups() const {
  std::vector<std::shared_ptr<AclGroup>> result;
  for (const auto& [id, group] : groups_) {
    result.push_back(group);
  }
  return result;
}

bool AclManager::check_right(uint32_t user_id, uint32_t channel_id, ChannelRight right) const {
  // Admins haben immer alle Rechte
  if (is_admin(user_id)) {
    return true;
  }

  // Prüfe alle Gruppen des Nutzers
  ChannelRight user_rights = ChannelRight::None;
  for (const auto& [id, group] : groups_) {
    if (group->has_member(user_id)) {
      user_rights = user_rights | group->get_rights(channel_id);
    }
  }

  return has_right(user_rights, right);
}

ChannelRight AclManager::get_user_rights(uint32_t user_id, uint32_t channel_id) const {
  // Admins haben immer alle Rechte
  if (is_admin(user_id)) {
    return static_cast<ChannelRight>(0xFFFF); // Alle Rechte
  }

  // Prüfe alle Gruppen des Nutzers
  ChannelRight user_rights = ChannelRight::None;
  for (const auto& [id, group] : groups_) {
    if (group->has_member(user_id)) {
      user_rights = user_rights | group->get_rights(channel_id);
    }
  }

  return user_rights;
}

void AclManager::set_user_admin(uint32_t user_id, bool admin) {
  admins_[user_id] = admin;
  hh::get_logger().info("AclManager: User {} admin={}", user_id, admin);
}

bool AclManager::is_admin(uint32_t user_id) const {
  auto it = admins_.find(user_id);
  return it != admins_.end() && it->second;
}

} // namespace hh::server

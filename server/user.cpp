// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/user.hpp"

#include "hh/logging.hpp"

namespace hh::server {

// ── User ──────────────────────────────────────────────────────────────

User::User(uint32_t id, const std::string& username,
           const std::shared_ptr<Connection>& connection)
  : id_(id)
  , username_(username)
  , connection_(connection)
  , channel_id_(0) {
}

// ── UserManager ───────────────────────────────────────────────────────

std::shared_ptr<User> UserManager::create_user(uint32_t id, const std::string& username,
                                                const std::shared_ptr<Connection>& connection) {
  auto user = std::make_shared<User>(id, username, connection);
  users_[id] = user;
  users_by_name_[username] = user;

  hh::get_logger().info("User created: {} (id: {})", username, id);
  return user;
}

std::shared_ptr<User> UserManager::find_user(uint32_t id) const {
  auto it = users_.find(id);
  if (it != users_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<User> UserManager::find_user_by_name(const std::string& name) const {
  auto it = users_by_name_.find(name);
  if (it != users_by_name_.end()) {
    return it->second;
  }
  return nullptr;
}

bool UserManager::remove_user(uint32_t id) {
  auto it = users_.find(id);
  if (it == users_.end()) {
    return false;
  }

  auto user = it->second;
  users_by_name_.erase(user->username_);
  users_.erase(it);

  hh::get_logger().info("User removed: {} (id: {})", user->username_, id);
  return true;
}

std::vector<std::shared_ptr<User>> UserManager::all_users() const {
  std::vector<std::shared_ptr<User>> result;
  for (const auto& [id, user] : users_) {
    result.push_back(user);
  }
  return result;
}

} // namespace hh::server

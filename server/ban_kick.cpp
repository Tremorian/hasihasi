// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/ban_kick.hpp"

#include "hh/logging.hpp"

#include <chrono>

namespace hh::server {

// ── BanEntry ──────────────────────────────────────────────────────────

BanEntry::BanEntry(uint32_t id, uint32_t user_id, const std::string& username,
                   uint32_t admin_id, const std::string& reason, uint64_t timestamp)
  : id_(id)
  , user_id_(user_id)
  , username_(username)
  , admin_id_(admin_id)
  , reason_(reason)
  , timestamp_(timestamp) {
}

// ── KickEntry ─────────────────────────────────────────────────────────

KickEntry::KickEntry(uint32_t id, uint32_t user_id, const std::string& username,
                     uint32_t admin_id, const std::string& reason, uint64_t timestamp)
  : id_(id)
  , user_id_(user_id)
  , username_(username)
  , admin_id_(admin_id)
  , reason_(reason)
  , timestamp_(timestamp) {
}

// ── BanKickManager ────────────────────────────────────────────────────

bool BanKickManager::is_banned(uint32_t user_id) const {
  for (const auto& ban : bans_) {
    if (ban->user_id() == user_id) {
      return true;
    }
  }
  return false;
}

std::shared_ptr<BanEntry> BanKickManager::add_ban(uint32_t user_id, const std::string& username,
                                                   uint32_t admin_id, const std::string& reason) {
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  auto ban = std::make_shared<BanEntry>(next_ban_id_, user_id, username, admin_id, reason, now);
  bans_.push_back(ban);
  next_ban_id_++;

  hh::get_logger().info("BanKickManager: User {} ({}) banned by admin {} (reason: {})",
                        user_id, username, admin_id, reason);
  return ban;
}

bool BanKickManager::remove_ban(uint32_t user_id) {
  for (auto it = bans_.begin(); it != bans_.end(); ++it) {
    if ((*it)->user_id() == user_id) {
      bans_.erase(it);
      hh::get_logger().info("BanKickManager: Ban for user {} removed", user_id);
      return true;
    }
  }
  return false;
}

std::vector<std::shared_ptr<BanEntry>> BanKickManager::get_bans() const {
  return bans_;
}

std::shared_ptr<KickEntry> BanKickManager::add_kick(uint32_t user_id, const std::string& username,
                                                     uint32_t admin_id, const std::string& reason) {
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  auto kick = std::make_shared<KickEntry>(next_kick_id_, user_id, username, admin_id, reason, now);
  kicks_.push_back(kick);
  next_kick_id_++;

  hh::get_logger().info("BanKickManager: User {} ({}) kicked by admin {} (reason: {})",
                        user_id, username, admin_id, reason);
  return kick;
}

std::vector<std::shared_ptr<KickEntry>> BanKickManager::get_kicks() const {
  return kicks_;
}

} // namespace hh::server

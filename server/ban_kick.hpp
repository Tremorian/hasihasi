// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace hh::server {

// Repräsentiert einen Ban
class BanEntry {
public:
  BanEntry(uint32_t id, uint32_t user_id, const std::string& username,
           uint32_t admin_id, const std::string& reason, uint64_t timestamp);

  // Eigenschaften
  uint32_t id() const { return id_; }
  uint32_t user_id() const { return user_id_; }
  const std::string& username() const { return username_; }
  uint32_t admin_id() const { return admin_id_; }
  const std::string& reason() const { return reason_; }
  uint64_t timestamp() const { return timestamp_; }

private:
  uint32_t id_;
  uint32_t user_id_;
  std::string username_;
  uint32_t admin_id_;
  std::string reason_;
  uint64_t timestamp_;
};

// Repräsentiert einen Kick-Eintrag
class KickEntry {
public:
  KickEntry(uint32_t id, uint32_t user_id, const std::string& username,
            uint32_t admin_id, const std::string& reason, uint64_t timestamp);

  // Eigenschaften
  uint32_t id() const { return id_; }
  uint32_t user_id() const { return user_id_; }
  const std::string& username() const { return username_; }
  uint32_t admin_id() const { return admin_id_; }
  const std::string& reason() const { return reason_; }
  uint64_t timestamp() const { return timestamp_; }

private:
  uint32_t id_;
  uint32_t user_id_;
  std::string username_;
  uint32_t admin_id_;
  std::string reason_;
  uint64_t timestamp_;
};

// Ban/Kick-Verwaltung für Admin-Funktionen
class BanKickManager {
public:
  BanKickManager() = default;

  // Ban-Operationen
  bool is_banned(uint32_t user_id) const;
  std::shared_ptr<BanEntry> add_ban(uint32_t user_id, const std::string& username,
                                     uint32_t admin_id, const std::string& reason);
  bool remove_ban(uint32_t user_id);
  std::vector<std::shared_ptr<BanEntry>> get_bans() const;

  // Kick-Operationen
  std::shared_ptr<KickEntry> add_kick(uint32_t user_id, const std::string& username,
                                       uint32_t admin_id, const std::string& reason);
  std::vector<std::shared_ptr<KickEntry>> get_kicks() const;

  // Statistik
  size_t ban_count() const { return bans_.size(); }
  size_t kick_count() const { return kicks_.size(); }

private:
  std::vector<std::shared_ptr<BanEntry>> bans_;
  std::vector<std::shared_ptr<KickEntry>> kicks_;
  uint32_t next_ban_id_ = 1;
  uint32_t next_kick_id_ = 1;
};

} // namespace hh::server

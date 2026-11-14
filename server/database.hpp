// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace hh::server {

// SQLite-Datenbank-Wrapper für Persistenz
class Database {
public:
  Database() = default;
  ~Database() = default;

  // Datenbank öffnen/schließen
  bool open(const std::string& filename);
  void close();
  bool is_open() const { return is_open_; }

  // Migrationen
  bool migrate();
  int current_migration() const { return current_migration_; }

  // Kanal-Operationen
  struct ChannelData {
    uint32_t id;
    uint32_t parent_id;
    std::string name;
    uint32_t max_users;
    bool temporary;
    std::string description;
  };

  bool save_channel(const ChannelData& channel);
  bool delete_channel(uint32_t channel_id);
  ChannelData load_channel(uint32_t channel_id) const;
  std::vector<ChannelData> load_all_channels() const;
  std::vector<ChannelData> load_channels_by_parent(uint32_t parent_id) const;

  // Nutzer-Operationen
  struct UserData {
    uint32_t id;
    std::string username;
    std::string cert_fingerprint; // Optional für Registrierung
    uint32_t channel_id;
    bool is_admin;
  };

  bool save_user(const UserData& user);
  bool delete_user(uint32_t user_id);
  UserData load_user(uint32_t user_id) const;
  UserData load_user_by_name(const std::string& username) const;
  UserData load_user_by_cert(const std::string& cert_fingerprint) const;
  std::vector<UserData> load_all_users() const;

  // ACL-Operationen
  struct AclGroupData {
    uint32_t id;
    std::string name;
    bool inherits;
  };

  struct AclRightData {
    uint32_t group_id;
    uint32_t channel_id;
    uint16_t rights; // Bitmask
  };

  struct AclMemberData {
    uint32_t group_id;
    uint32_t user_id;
  };

  bool save_acl_group(const AclGroupData& group);
  bool delete_acl_group(uint32_t group_id);
  AclGroupData load_acl_group(uint32_t group_id) const;
  std::vector<AclGroupData> load_all_acl_groups() const;

  bool save_acl_right(const AclRightData& right);
  bool delete_acl_right(uint32_t group_id, uint32_t channel_id);
  std::vector<AclRightData> load_acl_rights(uint32_t group_id) const;
  std::vector<AclRightData> load_acl_rights_for_channel(uint32_t channel_id) const;

  bool save_acl_member(const AclMemberData& member);
  bool delete_acl_member(uint32_t group_id, uint32_t user_id);
  std::vector<AclMemberData> load_acl_members(uint32_t group_id) const;
  std::vector<AclMemberData> load_acl_members_for_user(uint32_t user_id) const;

  // Transaktionen
  bool begin_transaction();
  bool commit_transaction();
  bool rollback_transaction();

  // Callbacks
  using RowCallback = std::function<void(const std::vector<std::string>&)>;
  bool execute_query(const std::string& sql, RowCallback callback = nullptr);
  bool execute_command(const std::string& sql);

private:
  bool create_tables();
  bool apply_migration(int migration);

  void* db_ = nullptr;
  int current_migration_ = 0;
  bool is_open_ = false;
};

} // namespace hh::server

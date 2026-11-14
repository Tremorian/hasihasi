// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/database.hpp"

#include "hh/logging.hpp"

#include <sqlite3.h>
#include <sstream>

namespace hh::server {

// ── Hilfsfunktionen ───────────────────────────────────────────────────

static int row_callback(void* data, int argc, char** argv, char** col_names) {
  auto& callback = *static_cast<Database::RowCallback*>(data);
  std::vector<std::string> row;
  for (int i = 0; i < argc; i++) {
    row.push_back(argv[i] ? argv[i] : "NULL");
  }
  callback(row);
  return 0;
}

// ── Database ──────────────────────────────────────────────────────────

bool Database::open(const std::string& filename) {
  int rc = sqlite3_open(filename.c_str(), static_cast<sqlite3**>(&db_));
  if (rc != SQLITE_OK) {
    hh::get_logger().error("Database: Failed to open database: {}", sqlite3_errmsg(db_));
    return false;
  }

  // PRAGMA Einstellungen für Performance und Sicherheit
  execute_command("PRAGMA journal_mode=WAL");
  execute_command("PRAGMA synchronous=NORMAL");
  execute_command("PRAGMA foreign_keys=ON");

  is_open_ = true;
  hh::get_logger().info("Database: Opened database '{}'", filename);
  return true;
}

void Database::close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
  is_open_ = false;
  hh::get_logger().info("Database: Closed database");
}

bool Database::migrate() {
  // Migrationen anwenden
  for (int i = current_migration_ + 1; i <= 1; i++) {
    if (!apply_migration(i)) {
      return false;
    }
  }
  return true;
}

bool Database::create_tables() {
  const char* sql = R"(
    -- Kanäle
    CREATE TABLE IF NOT EXISTS channels (
      id INTEGER PRIMARY KEY,
      parent_id INTEGER NOT NULL,
      name TEXT NOT NULL,
      max_users INTEGER NOT NULL DEFAULT 0,
      temporary INTEGER NOT NULL DEFAULT 0,
      description TEXT NOT NULL DEFAULT '',
      FOREIGN KEY (parent_id) REFERENCES channels(id)
    );

    -- Nutzer
    CREATE TABLE IF NOT EXISTS users (
      id INTEGER PRIMARY KEY,
      username TEXT NOT NULL UNIQUE,
      cert_fingerprint TEXT,
      channel_id INTEGER,
      is_admin INTEGER NOT NULL DEFAULT 0,
      FOREIGN KEY (channel_id) REFERENCES channels(id)
    );

    -- ACL-Gruppen
    CREATE TABLE IF NOT EXISTS acl_groups (
      id INTEGER PRIMARY KEY,
      name TEXT NOT NULL UNIQUE,
      inherits INTEGER NOT NULL DEFAULT 0
    );

    -- ACL-Rechte (pro Gruppe und Kanal)
    CREATE TABLE IF NOT EXISTS acl_rights (
      group_id INTEGER NOT NULL,
      channel_id INTEGER NOT NULL,
      rights INTEGER NOT NULL,
      PRIMARY KEY (group_id, channel_id),
      FOREIGN KEY (group_id) REFERENCES acl_groups(id),
      FOREIGN KEY (channel_id) REFERENCES channels(id)
    );

    -- ACL-Mitglieder
    CREATE TABLE IF NOT EXISTS acl_members (
      group_id INTEGER NOT NULL,
      user_id INTEGER NOT NULL,
      PRIMARY KEY (group_id, user_id),
      FOREIGN KEY (group_id) REFERENCES acl_groups(id),
      FOREIGN KEY (user_id) REFERENCES users(id)
    );

    -- Migrationen
    CREATE TABLE IF NOT EXISTS migrations (
      id INTEGER PRIMARY KEY
    );
  )";

  char* err_msg = nullptr;
  int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    hh::get_logger().error("Database: Failed to create tables: {}", err_msg);
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

bool Database::apply_migration(int migration) {
  hh::get_logger().info("Database: Applying migration {}", migration);

  // Migration 1: Erstellt alle Tabellen
  if (!create_tables()) {
    return false;
  }

  // Migration markieren
  std::stringstream sql;
  sql << "INSERT INTO migrations (id) VALUES (" << migration << ")";
  if (!execute_command(sql.str())) {
    return false;
  }

  current_migration_ = migration;
  hh::get_logger().info("Database: Migration {} applied", migration);
  return true;
}

// ── Kanal-Operationen ─────────────────────────────────────────────────

bool Database::save_channel(const ChannelData& channel) {
  std::stringstream sql;
  sql << "INSERT OR REPLACE INTO channels (id, parent_id, name, max_users, temporary, description) "
      << "VALUES ("
      << channel.id << ", "
      << channel.parent_id << ", '"
      << channel.name << "', "
      << channel.max_users << ", "
      << (channel.temporary ? 1 : 0) << ", '"
      << channel.description << "')";
  return execute_command(sql.str());
}

bool Database::delete_channel(uint32_t channel_id) {
  std::stringstream sql;
  sql << "DELETE FROM channels WHERE id = " << channel_id;
  return execute_command(sql.str());
}

Database::ChannelData Database::load_channel(uint32_t channel_id) const {
  ChannelData channel{};
  std::stringstream sql;
  sql << "SELECT id, parent_id, name, max_users, temporary, description FROM channels WHERE id = " << channel_id;

  execute_query(sql.str(), [&channel](const std::vector<std::string>& row) {
    if (row.size() >= 6) {
      channel.id = std::stoul(row[0]);
      channel.parent_id = std::stoul(row[1]);
      channel.name = row[2];
      channel.max_users = std::stoul(row[3]);
      channel.temporary = std::stoul(row[4]) != 0;
      channel.description = row[5];
    }
  });

  return channel;
}

std::vector<Database::ChannelData> Database::load_all_channels() const {
  std::vector<ChannelData> channels;
  std::stringstream sql;
  sql << "SELECT id, parent_id, name, max_users, temporary, description FROM channels";

  execute_query(sql.str(), [&channels](const std::vector<std::string>& row) {
    ChannelData channel{};
    if (row.size() >= 6) {
      channel.id = std::stoul(row[0]);
      channel.parent_id = std::stoul(row[1]);
      channel.name = row[2];
      channel.max_users = std::stoul(row[3]);
      channel.temporary = std::stoul(row[4]) != 0;
      channel.description = row[5];
      channels.push_back(channel);
    }
  });

  return channels;
}

std::vector<Database::ChannelData> Database::load_channels_by_parent(uint32_t parent_id) const {
  std::vector<ChannelData> channels;
  std::stringstream sql;
  sql << "SELECT id, parent_id, name, max_users, temporary, description FROM channels WHERE parent_id = " << parent_id;

  execute_query(sql.str(), [&channels](const std::vector<std::string>& row) {
    ChannelData channel{};
    if (row.size() >= 6) {
      channel.id = std::stoul(row[0]);
      channel.parent_id = std::stoul(row[1]);
      channel.name = row[2];
      channel.max_users = std::stoul(row[3]);
      channel.temporary = std::stoul(row[4]) != 0;
      channel.description = row[5];
      channels.push_back(channel);
    }
  });

  return channels;
}

// ── Nutzer-Operationen ────────────────────────────────────────────────

bool Database::save_user(const UserData& user) {
  std::stringstream sql;
  sql << "INSERT OR REPLACE INTO users (id, username, cert_fingerprint, channel_id, is_admin) "
      << "VALUES ("
      << user.id << ", '"
      << user.username << "', "
      << (user.cert_fingerprint.empty() ? "NULL" : "'" + user.cert_fingerprint + "'") << ", "
      << user.channel_id << ", "
      << (user.is_admin ? 1 : 0) << ")";
  return execute_command(sql.str());
}

bool Database::delete_user(uint32_t user_id) {
  std::stringstream sql;
  sql << "DELETE FROM users WHERE id = " << user_id;
  return execute_command(sql.str());
}

Database::UserData Database::load_user(uint32_t user_id) const {
  UserData user{};
  std::stringstream sql;
  sql << "SELECT id, username, cert_fingerprint, channel_id, is_admin FROM users WHERE id = " << user_id;

  execute_query(sql.str(), [&user](const std::vector<std::string>& row) {
    if (row.size() >= 5) {
      user.id = std::stoul(row[0]);
      user.username = row[1];
      user.cert_fingerprint = (row[2] != "NULL") ? row[2] : "";
      user.channel_id = std::stoul(row[3]);
      user.is_admin = std::stoul(row[4]) != 0;
    }
  });

  return user;
}

Database::UserData Database::load_user_by_name(const std::string& username) const {
  UserData user{};
  std::stringstream sql;
  sql << "SELECT id, username, cert_fingerprint, channel_id, is_admin FROM users WHERE username = '" << username << "'";

  execute_query(sql.str(), [&user](const std::vector<std::string>& row) {
    if (row.size() >= 5) {
      user.id = std::stoul(row[0]);
      user.username = row[1];
      user.cert_fingerprint = (row[2] != "NULL") ? row[2] : "";
      user.channel_id = std::stoul(row[3]);
      user.is_admin = std::stoul(row[4]) != 0;
    }
  });

  return user;
}

Database::UserData Database::load_user_by_cert(const std::string& cert_fingerprint) const {
  UserData user{};
  std::stringstream sql;
  sql << "SELECT id, username, cert_fingerprint, channel_id, is_admin FROM users WHERE cert_fingerprint = '" << cert_fingerprint << "'";

  execute_query(sql.str(), [&user](const std::vector<std::string>& row) {
    if (row.size() >= 5) {
      user.id = std::stoul(row[0]);
      user.username = row[1];
      user.cert_fingerprint = (row[2] != "NULL") ? row[2] : "";
      user.channel_id = std::stoul(row[3]);
      user.is_admin = std::stoul(row[4]) != 0;
    }
  });

  return user;
}

std::vector<Database::UserData> Database::load_all_users() const {
  std::vector<UserData> users;
  std::stringstream sql;
  sql << "SELECT id, username, cert_fingerprint, channel_id, is_admin FROM users";

  execute_query(sql.str(), [&users](const std::vector<std::string>& row) {
    UserData user{};
    if (row.size() >= 5) {
      user.id = std::stoul(row[0]);
      user.username = row[1];
      user.cert_fingerprint = (row[2] != "NULL") ? row[2] : "";
      user.channel_id = std::stoul(row[3]);
      user.is_admin = std::stoul(row[4]) != 0;
      users.push_back(user);
    }
  });

  return users;
}

// ── ACL-Operationen ───────────────────────────────────────────────────

bool Database::save_acl_group(const AclGroupData& group) {
  std::stringstream sql;
  sql << "INSERT OR REPLACE INTO acl_groups (id, name, inherits) "
      << "VALUES ("
      << group.id << ", '"
      << group.name << "', "
      << (group.inherits ? 1 : 0) << ")";
  return execute_command(sql.str());
}

bool Database::delete_acl_group(uint32_t group_id) {
  std::stringstream sql;
  sql << "DELETE FROM acl_groups WHERE id = " << group_id;
  return execute_command(sql.str());
}

Database::AclGroupData Database::load_acl_group(uint32_t group_id) const {
  AclGroupData group{};
  std::stringstream sql;
  sql << "SELECT id, name, inherits FROM acl_groups WHERE id = " << group_id;

  execute_query(sql.str(), [&group](const std::vector<std::string>& row) {
    if (row.size() >= 3) {
      group.id = std::stoul(row[0]);
      group.name = row[1];
      group.inherits = std::stoul(row[2]) != 0;
    }
  });

  return group;
}

std::vector<Database::AclGroupData> Database::load_all_acl_groups() const {
  std::vector<AclGroupData> groups;
  std::stringstream sql;
  sql << "SELECT id, name, inherits FROM acl_groups";

  execute_query(sql.str(), [&groups](const std::vector<std::string>& row) {
    AclGroupData group{};
    if (row.size() >= 3) {
      group.id = std::stoul(row[0]);
      group.name = row[1];
      group.inherits = std::stoul(row[2]) != 0;
      groups.push_back(group);
    }
  });

  return groups;
}

bool Database::save_acl_right(const AclRightData& right) {
  std::stringstream sql;
  sql << "INSERT OR REPLACE INTO acl_rights (group_id, channel_id, rights) "
      << "VALUES ("
      << right.group_id << ", "
      << right.channel_id << ", "
      << right.rights << ")";
  return execute_command(sql.str());
}

bool Database::delete_acl_right(uint32_t group_id, uint32_t channel_id) {
  std::stringstream sql;
  sql << "DELETE FROM acl_rights WHERE group_id = " << group_id << " AND channel_id = " << channel_id;
  return execute_command(sql.str());
}

std::vector<Database::AclRightData> Database::load_acl_rights(uint32_t group_id) const {
  std::vector<AclRightData> rights;
  std::stringstream sql;
  sql << "SELECT group_id, channel_id, rights FROM acl_rights WHERE group_id = " << group_id;

  execute_query(sql.str(), [&rights](const std::vector<std::string>& row) {
    AclRightData right{};
    if (row.size() >= 3) {
      right.group_id = std::stoul(row[0]);
      right.channel_id = std::stoul(row[1]);
      right.rights = static_cast<uint16_t>(std::stoul(row[2]));
      rights.push_back(right);
    }
  });

  return rights;
}

std::vector<Database::AclRightData> Database::load_acl_rights_for_channel(uint32_t channel_id) const {
  std::vector<AclRightData> rights;
  std::stringstream sql;
  sql << "SELECT group_id, channel_id, rights FROM acl_rights WHERE channel_id = " << channel_id;

  execute_query(sql.str(), [&rights](const std::vector<std::string>& row) {
    AclRightData right{};
    if (row.size() >= 3) {
      right.group_id = std::stoul(row[0]);
      right.channel_id = std::stoul(row[1]);
      right.rights = static_cast<uint16_t>(std::stoul(row[2]));
      rights.push_back(right);
    }
  });

  return rights;
}

bool Database::save_acl_member(const AclMemberData& member) {
  std::stringstream sql;
  sql << "INSERT OR REPLACE INTO acl_members (group_id, user_id) "
      << "VALUES ("
      << member.group_id << ", "
      << member.user_id << ")";
  return execute_command(sql.str());
}

bool Database::delete_acl_member(uint32_t group_id, uint32_t user_id) {
  std::stringstream sql;
  sql << "DELETE FROM acl_members WHERE group_id = " << group_id << " AND user_id = " << user_id;
  return execute_command(sql.str());
}

std::vector<Database::AclMemberData> Database::load_acl_members(uint32_t group_id) const {
  std::vector<AclMemberData> members;
  std::stringstream sql;
  sql << "SELECT group_id, user_id FROM acl_members WHERE group_id = " << group_id;

  execute_query(sql.str(), [&members](const std::vector<std::string>& row) {
    AclMemberData member{};
    if (row.size() >= 2) {
      member.group_id = std::stoul(row[0]);
      member.user_id = std::stoul(row[1]);
      members.push_back(member);
    }
  });

  return members;
}

std::vector<Database::AclMemberData> Database::load_acl_members_for_user(uint32_t user_id) const {
  std::vector<AclMemberData> members;
  std::stringstream sql;
  sql << "SELECT group_id, user_id FROM acl_members WHERE user_id = " << user_id;

  execute_query(sql.str(), [&members](const std::vector<std::string>& row) {
    AclMemberData member{};
    if (row.size() >= 2) {
      member.group_id = std::stoul(row[0]);
      member.user_id = std::stoul(row[1]);
      members.push_back(member);
    }
  });

  return members;
}

// ── Transaktionen ─────────────────────────────────────────────────────

bool Database::begin_transaction() {
  return execute_command("BEGIN TRANSACTION");
}

bool Database::commit_transaction() {
  return execute_command("COMMIT TRANSACTION");
}

bool Database::rollback_transaction() {
  return execute_command("ROLLBACK TRANSACTION");
}

// ── Query/Command ─────────────────────────────────────────────────────

bool Database::execute_query(const std::string& sql, RowCallback callback) {
  char* err_msg = nullptr;

  if (callback) {
    int rc = sqlite3_exec(db_, sql.c_str(), row_callback, static_cast<void*>(&callback), &err_msg);
    if (rc != SQLITE_OK) {
      hh::get_logger().error("Database: Query failed: {}", err_msg);
      sqlite3_free(err_msg);
      return false;
    }
  } else {
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
      hh::get_logger().error("Database: Query failed: {}", err_msg);
      sqlite3_free(err_msg);
      return false;
    }
  }

  return true;
}

bool Database::execute_command(const std::string& sql) {
  char* err_msg = nullptr;
  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    hh::get_logger().error("Database: Command failed: {}", err_msg);
    sqlite3_free(err_msg);
    return false;
  }
  return true;
}

} // namespace hh::server

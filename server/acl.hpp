// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

namespace hh::server {

// ACL-Rechte pro Kanal (bitmask)
enum class ChannelRight : uint16_t {
  None = 0,
  Join = 1 << 0,        // Kanal betreten
  Speak = 1 << 1,       // Sprechen
  Whisper = 1 << 2,     // Flüstern (an bestimmte Nutzer/Kanäle)
  Text = 1 << 3,        // Textnachrichten senden
  Move = 1 << 4,        // Kanäle verschieben
  Mute = 1 << 5,        // Andere muten
  Kick = 1 << 6,        // Andere kicken
  Ban = 1 << 7,         // Andere bannen
  CreateChannel = 1 << 8, // Neue Kanäle erstellen
  Register = 1 << 9     // Registrierte Nutzer erstellen
};

inline ChannelRight operator|(ChannelRight a, ChannelRight b) {
  return static_cast<ChannelRight>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline ChannelRight operator&(ChannelRight a, ChannelRight b) {
  return static_cast<ChannelRight>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline bool has_right(ChannelRight rights, ChannelRight right) {
  return (rights & right) != ChannelRight::None;
}

// Gruppe mit vererbbaren Rechten
class AclGroup {
public:
  AclGroup(uint32_t id, const std::string& name);

  // Eigenschaften
  uint32_t id() const { return id_; }
  const std::string& name() const { return name_; }

  // Rechte setzen
  void set_rights(uint32_t channel_id, ChannelRight rights);
  ChannelRight get_rights(uint32_t channel_id) const;

  // Vererbung
  void set_inherits(bool inherits) { inherits_ = inherits; }
  bool inherits() const { return inherits_; }

  // Mitglieder
  void add_member(uint32_t user_id);
  void remove_member(uint32_t user_id);
  std::vector<uint32_t> members() const { return members_; }
  bool has_member(uint32_t user_id) const;

private:
  uint32_t id_;
  std::string name_;
  bool inherits_ = false;
  std::map<uint32_t, ChannelRight> channel_rights_; // channel_id -> rights
  std::vector<uint32_t> members_;
};

// ACL-Manager für Kanal-basierte Rechte
class AclManager {
public:
  AclManager() = default;

  // Gruppen-Verwaltung
  std::shared_ptr<AclGroup> create_group(const std::string& name, bool inherits = false);
  bool delete_group(uint32_t group_id);
  std::shared_ptr<AclGroup> find_group(uint32_t group_id) const;
  std::shared_ptr<AclGroup> find_group_by_name(const std::string& name) const;
  std::vector<std::shared_ptr<AclGroup>> all_groups() const;

  // Rechte-Prüfung
  bool check_right(uint32_t user_id, uint32_t channel_id, ChannelRight right) const;
  ChannelRight get_user_rights(uint32_t user_id, uint32_t channel_id) const;

  // Admin-Rechte (immer wahr für Admins)
  void set_user_admin(uint32_t user_id, bool admin);
  bool is_admin(uint32_t user_id) const;

private:
  std::map<uint32_t, std::shared_ptr<AclGroup>> groups_;
  std::map<uint32_t, bool> admins_; // user_id -> is_admin
};

} // namespace hh::server

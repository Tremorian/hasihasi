// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/whisper_shout.hpp"

#include "hh/logging.hpp"

namespace hh::server {

// ── VoiceTargetManager ────────────────────────────────────────────────

void VoiceTargetManager::set_voice_target(uint32_t user_id, const VoiceTarget& target) {
  auto old_target = targets_.find(user_id);
  bool was_whispering = old_target != targets_.end() && old_target->second.is_whisper();
  bool was_shouting = old_target != targets_.end() && old_target->second.is_shout();

  // Alte Einträge entfernen
  if (was_whispering) {
    whispering_.erase(user_id);
  }
  if (was_shouting) {
    shouting_.erase(user_id);
  }

  // Neuen Eintrag setzen
  targets_[user_id] = target;

  // Neue Einträge hinzufügen
  if (target.is_whisper()) {
    whispering_[user_id] = target.target_id();
    hh::get_logger().info("VoiceTargetManager: User {} whispering to {}", user_id, target.target_id());
  } else if (target.is_shout()) {
    shouting_[user_id] = 0; // Channel-ID wird später gesetzt
    hh::get_logger().info("VoiceTargetManager: User {} shouting", user_id);
  }
}

VoiceTarget VoiceTargetManager::get_voice_target(uint32_t user_id) const {
  auto it = targets_.find(user_id);
  if (it == targets_.end()) {
    return VoiceTarget();
  }
  return it->second;
}

void VoiceTargetManager::clear_voice_target(uint32_t user_id) {
  auto it = targets_.find(user_id);
  if (it == targets_.end()) {
    return;
  }

  auto target = it->second;
  if (target.is_whispering()) {
    whispering_.erase(user_id);
  }
  if (target.is_shouting()) {
    shouting_.erase(user_id);
  }

  targets_.erase(it);
  hh::get_logger().info("VoiceTargetManager: Voice target for user {} cleared", user_id);
}

bool VoiceTargetManager::is_whispering(uint32_t user_id) const {
  return whispering_.find(user_id) != whispering_.end();
}

bool VoiceTargetManager::is_shouting(uint32_t user_id) const {
  return shouting_.find(user_id) != shouting_.end();
}

uint32_t VoiceTargetManager::whisper_target(uint32_t user_id) const {
  auto it = whispering_.find(user_id);
  if (it == whispering_.end()) {
    return 0;
  }
  return it->second;
}

std::vector<uint32_t> VoiceTargetManager::whispering_users() const {
  std::vector<uint32_t> users;
  for (const auto& [user_id, _] : whispering_) {
    users.push_back(user_id);
  }
  return users;
}

std::vector<uint32_t> VoiceTargetManager::shouting_users() const {
  std::vector<uint32_t> users;
  for (const auto& [user_id, _] : shouting_) {
    users.push_back(user_id);
  }
  return users;
}

} // namespace hh::server

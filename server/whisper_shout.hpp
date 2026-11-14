// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace hh::server {

// Repräsentiert eine Whisper/Shout-Konfiguration
class VoiceTarget {
public:
  enum class Type {
    NONE,    // Normal (alle hören)
    WHISPER, // Flüstern (nur Ziel hört)
    SHOUT    // Rufen (alle hören, auch außerhalb des Kanals)
  };

  VoiceTarget() : type_(Type::NONE) {}
  VoiceTarget(Type type, uint32_t target_id = 0) : type_(type), target_id_(target_id) {}

  // Eigenschaften
  Type type() const { return type_; }
  uint32_t target_id() const { return target_id_; }

  // Typ-Prüfung
  bool is_none() const { return type_ == Type::NONE; }
  bool is_whisper() const { return type_ == Type::WHISPER; }
  bool is_shout() const { return type_ == Type::SHOUT; }

  // Konfiguration ändern
  void set_none() { type_ = Type::NONE; }
  void set_whisper(uint32_t target_id) { type_ = Type::WHISPER; target_id_ = target_id; }
  void set_shout() { type_ = Type::SHOUT; }

private:
  Type type_;
  uint32_t target_id_ = 0;
};

// Whisper/Shout-Verwaltung für selektive Audio-Ziele
class VoiceTargetManager {
public:
  VoiceTargetManager() = default;

  // Whisper/Shout für Nutzer setzen
  void set_voice_target(uint32_t user_id, const VoiceTarget& target);

  // Whisper/Shout für Nutzer abrufen
  VoiceTarget get_voice_target(uint32_t user_id) const;

  // Whisper/Shoot entfernen
  void clear_voice_target(uint32_t user_id);

  // Prüfen ob Whisper/Shout aktiv
  bool is_whispering(uint32_t user_id) const;
  bool is_shouting(uint32_t user_id) const;

  // Whisper-Ziel für Nutzer abrufen
  uint32_t whisper_target(uint32_t user_id) const;

  // Alle Nutzer mit Whisper/Shout abrufen
  std::vector<uint32_t> whispering_users() const;
  std::vector<uint32_t> shouting_users() const;

  // Statistik
  size_t whisper_count() const { return whispering_.size(); }
  size_t shout_count() const { return shouting_.size(); }

private:
  std::map<uint32_t, VoiceTarget> targets_;
  std::map<uint32_t, uint32_t> whispering_; // user_id -> whisper_target
  std::map<uint32_t, uint32_t> shouting_;    // user_id -> channel_id (vereinfacht)
};

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server_i18n.hpp"

#include <algorithm>

namespace hh::server {

// ── ServerI18n ────────────────────────────────────────────────────────

ServerI18n::ServerI18n(ServerI18n&& other) noexcept
  : language_(other.language_)
  , translations_(std::move(other.translations_)) {
  
  other.language_ = Language::English;
}

ServerI18n& ServerI18n::operator=(ServerI18n&& other) noexcept {
  if (this != &other) {
    language_ = other.language_;
    translations_ = std::move(other.translations_);
    other.language_ = Language::English;
  }
  return *this;
}

void ServerI18n::set_language(Language language) {
  std::lock_guard<std::mutex> lock(mutex_);
  language_ = language;
}

std::string ServerI18n::tr(const char* key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = translations_.find(key);
  if (it != translations_.end()) {
    return it->second;
  }
  
  // Fallback auf Englisch
  return key;
}

std::string ServerI18n::get_welcome_message() const {
  return tr("welcome.message");
}

std::string ServerI18n::get_error_message(const char* error_code) const {
  return tr(std::string("error.") + error_code);
}

std::string ServerI18n::get_status_message(const char* status_code) const {
  return tr(std::string("status.") + status_code);
}

std::string ServerI18n::language_to_code(Language language) {
  switch (language) {
    case Language::German: return "de";
    case Language::English: return "en";
    case Language::French: return "fr";
    case Language::Spanish: return "es";
    default: return "en";
  }
}

ServerI18n::Language ServerI18n::code_to_language(const std::string& code) {
  if (code == "de") {
    return Language::German;
  } else if (code == "en") {
    return Language::English;
  } else if (code == "fr") {
    return Language::French;
  } else if (code == "es") {
    return Language::Spanish;
  }
  return Language::English;
}

void ServerI18n::initialize_translations() {
  // Deutsche Übersetzungen
  translations_["welcome.message"] = "Willkommen bei HasiHasi!";
  translations_["error.auth_failed"] = "Authentifizung fehlgeschlagen";
  translations_["error.channel_not_found"] = "Kanal nicht gefunden";
  translations_["status.connected"] = "Verbunden";
  translations_["status.disconnected"] = "Getrennt";
  
  // Englische Übersetzungen (Fallback)
  translations_["en.welcome.message"] = "Welcome to HasiHasi!";
  translations_["en.error.auth_failed"] = "Authentication failed";
  translations_["en.error.channel_not_found"] = "Channel not found";
  translations_["en.status.connected"] = "Connected";
  translations_["en.status.disconnected"] = "Disconnected";
  
  // Französische Übersetzungen
  translations_["fr.welcome.message"] = "Bienvenue sur HasiHasi!";
  translations_["fr.error.auth_failed"] = "Échec de l'authentification";
  translations_["fr.error.channel_not_found"] = "Canal non trouvé";
  translations_["fr.status.connected"] = "Connecté";
  translations_["fr.status.disconnected"] = "Déconnecté";
  
  // Spanische Übersetzungen
  translations_["es.welcome.message"] = "¡Bienvenido a HasiHasi!";
  translations_["es.error.auth_failed"] = "Autenticación fallida";
  translations_["es.error.channel_not_found"] = "Canal no encontrado";
  translations_["es.status.connected"] = "Conectado";
  translations_["es.status.disconnected"] = "Desconectado";
}

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace hh::server {

// Server Internationalization (i18n)
// Unterstützt Deutsch, Englisch, Französisch, Spanisch
class ServerI18n {
public:
  enum class Language {
    German = 0,
    English,
    French,
    Spanish,
    Count
  };

  ServerI18n() = default;
  ~ServerI18n() = default;

  // Nicht kopierbar, bewegbar
  ServerI18n(const ServerI18n&) = delete;
  ServerI18n& operator=(const ServerI18n&) = delete;
  ServerI18n(ServerI18n&&) noexcept;
  ServerI18n& operator=(ServerI18n&&) noexcept;

  // Sprache setzen
  void set_language(Language language);
  Language get_language() const { return language_; }

  // Übersetzungsfunktion
  std::string tr(const char* key) const;

  // Standard-Meldungen
  std::string get_welcome_message() const;
  std::string get_error_message(const char* error_code) const;
  std::string get_status_message(const char* status_code) const;

  // Sprach-Codes
  static std::string language_to_code(Language language);
  static Language code_to_language(const std::string& code);

private:
  // Übersetzungen initialisieren
  void initialize_translations();

  // Spracheinstellung
  Language language_ = Language::English;

  // Übersetzungen
  std::unordered_map<std::string, std::string> translations_;

  // Mutex für Thread-Sicherheit
  mutable std::mutex mutex_;
};

} // namespace hh::server

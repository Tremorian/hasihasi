// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QTranslator>
#include <QLocale>
#include <QString>
#include <QDir>
#include <QApplication>
#include <memory>
#include <vector>
#include <functional>

namespace hh::client {

// Internationalization (i18n) Manager
// Verwendet Qt Linguist für Übersetzungen
class I18nManager : public QObject {
  Q_OBJECT

public:
  explicit I18nManager(QObject* parent = nullptr);
  ~I18nManager() override = default;

  // Nicht kopierbar, bewegbar
  I18nManager(const I18nManager&) = delete;
  I18nManager& operator=(const I18nManager&) = delete;
  I18nManager(I18nManager&&) noexcept;
  I18nManager& operator=(I18nManager&&) noexcept;

  // Unterstützte Sprachen
  enum class Language {
    German = 0,
    English,
    French,
    Spanish,
    AutoDetect,  // System-Sprache automatisch erkennen
    Count        // Anzahl der Sprachen
  };
  Q_ENUM(Language)

  // Spracheinstellungen abrufen/setzen
  Language current_language() const { return current_language_; }
  void set_language(Language language);
  Language auto_detect_language() const;

  // Übersetzungsfunktion
  QString translate(const char* context, const char* key, const char* disambiguation = nullptr, int n = -1) const;
  QString tr(const char* key, const char* disambiguation = nullptr, int n = -1) const;

  // Translation-Files verwalten
  bool load_translation(const QString& language_code);
  bool unload_translation(const QString& language_code);
  void reload_all_translations();

  // Verzeichnis für Translation-Files setzen
  void set_translation_directory(const QString& path);
  QString translation_directory() const { return translation_directory_; }

  // Callbacks
  using LanguageChangedCallback = std::function<void(Language language)>;
  void set_language_changed_callback(LanguageChangedCallback callback);

  // Statistiken
  struct TranslationStats {
    int total_strings = 0;
    int translated_strings = 0;
    int untranslated_strings = 0;
    double coverage_percentage = 0.0;
  };

  TranslationStats get_coverage_stats() const;

signals:
  void language_changed(Language language);
  void translation_loaded(const QString& language_code);
  void translation_unloaded(const QString& language_code);

private:
  // Sprache automatisch erkennen
  Language detect_system_language() const;
  
  // Sprach-Code zu Language-Enum konvertieren
  Language language_code_to_enum(const QString& code) const;
  
  // Language-Enum zu Sprach-Code konvertieren
  QString language_enum_to_code(Language language) const;
  
  // Translation-File laden
  bool load_translation_file(const QString& file_path);
  
  // Translation-File-Pfad konstruieren
  QString translation_file_path(const QString& language_code) const;
  
  // Verfügbare Translation-Files auflisten
  std::vector<QString> available_translations() const;

  // Einstellungen
  Language current_language_ = Language::AutoDetect;
  QString translation_directory_;
  
  // Translators
  std::unique_ptr<QTranslator> main_translator_;
  std::unique_ptr<QTranslator> qt_translator_;
  
  // Callbacks
  LanguageChangedCallback language_changed_callback_;
};

} // namespace hh::client

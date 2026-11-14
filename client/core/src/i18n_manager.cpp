// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/i18n_manager.hpp"

#include "hh/logging.hpp"

#include <QCoreApplication>
#include <QLocale>
#include <QDirIterator>
#include <QFileInfo>

namespace hh::client {

// ── I18nManager ───────────────────────────────────────────────────────

I18nManager::I18nManager(QObject* parent)
  : QObject(parent)
  , current_language_(Language::AutoDetect)
  , translation_directory_("")
  , main_translator_(nullptr)
  , qt_translator_(nullptr) {
  
  hh::get_logger().info("I18nManager: Initialized");
}

I18nManager::I18nManager(I18nManager&& other) noexcept
  : QObject(other.parent())
  , current_language_(other.current_language_)
  , translation_directory_(std::move(other.translation_directory_))
  , main_translator_(std::move(other.main_translator_))
  , qt_translator_(std::move(other.qt_translator_))
  , language_changed_callback_(std::move(other.language_changed_callback_)) {
  
  // QObject kann nicht verschoben werden, daher muss der Parent neu gesetzt werden
  // In diesem Fall bleibt der Parent unverändert
}

I18nManager& I18nManager::operator=(I18nManager&& other) noexcept {
  if (this != &other) {
    current_language_ = other.current_language_;
    translation_directory_ = std::move(other.translation_directory_);
    main_translator_ = std::move(other.main_translator_);
    qt_translator_ = std::move(other.qt_translator_);
    language_changed_callback_ = std::move(other.language_changed_callback_);
  }
  return *this;
}

void I18nManager::set_language(Language language) {
  if (current_language_ == language) {
    return;
  }
  
  current_language_ = language;
  
  // Sprache automatisch erkennen
  if (language == Language::AutoDetect) {
    language = detect_system_language();
  }
  
  // Translation laden
  QString language_code = language_enum_to_code(language);
  load_translation(language_code);
  
  // Callback aufrufen
  if (language_changed_callback_) {
    language_changed_callback_(language);
  }
  
  // Signal senden
  emit language_changed(language);
  
  hh::get_logger().info("I18nManager: Language changed to {}", qPrintable(language_code));
}

I18nManager::Language I18nManager::auto_detect_language() const {
  return detect_system_language();
}

QString I18nManager::translate(const char* context, const char* key, const char* disambiguation, int n) const {
  return QCoreApplication::translate(context, key, disambiguation, n);
}

QString I18nManager::tr(const char* key, const char* disambiguation, int n) const {
  return QCoreApplication::translate("hh::client::I18nManager", key, disambiguation, n);
}

bool I18nManager::load_translation(const QString& language_code) {
  // Alte Translation entladen
  if (main_translator_) {
    QCoreApplication::removeTranslator(main_translator_.get());
    main_translator_.reset();
  }
  
  // Translation-File-Pfad konstruieren
  QString file_path = translation_file_path(language_code);
  
  // Translation-File laden
  if (!load_translation_file(file_path)) {
    hh::get_logger().warn("I18nManager: Failed to load translation: {}", qPrintable(file_path));
    return false;
  }
  
  // Signal senden
  emit translation_loaded(language_code);
  
  return true;
}

bool I18nManager::unload_translation(const QString& language_code) {
  if (main_translator_) {
    QCoreApplication::removeTranslator(main_translator_.get());
    main_translator_.reset();
    emit translation_unloaded(language_code);
    return true;
  }
  return false;
}

void I18nManager::reload_all_translations() {
  // Alle Translailer entfernen
  if (main_translator_) {
    QCoreApplication::removeTranslator(main_translator_.get());
    main_translator_.reset();
  }
  if (qt_translator_) {
    QCoreApplication::removeTranslator(qt_translator_.get());
    qt_translator_.reset();
  }
  
  // Aktuelle Sprache neu laden
  Language language = current_language_;
  if (language == Language::AutoDetect) {
    language = detect_system_language();
  }
  
  QString language_code = language_enum_to_code(language);
  load_translation(language_code);
}

void I18nManager::set_translation_directory(const QString& path) {
  translation_directory_ = path;
  hh::get_logger().info("I18nManager: Translation directory set to {}", qPrintable(path));
}

void I18nManager::set_language_changed_callback(LanguageChangedCallback callback) {
  language_changed_callback_ = std::move(callback);
}

I18nManager::TranslationStats I18nManager::get_coverage_stats() const {
  TranslationStats stats;
  stats.total_strings = 0;
  stats.translated_strings = 0;
  stats.untranslated_strings = 0;
  stats.coverage_percentage = 0.0;
  
  // Einfache Statistik: Alle Strings im Code zählen
  // In einer echten Implementierung würde man die .ts-Files parsen
  return stats;
}

I18nManager::Language I18nManager::detect_system_language() const {
  QLocale locale = QLocale::system();
  QString language_code = locale.name().left(2);
  
  return language_code_to_enum(language_code);
}

I18nManager::Language I18nManager::language_code_to_enum(const QString& code) const {
  if (code == "de") {
    return Language::German;
  } else if (code == "en") {
    return Language::English;
  } else if (code == "fr") {
    return Language::French;
  } else if (code == "es") {
    return Language::Spanish;
  }
  
  // Fallback auf Englisch
  return Language::English;
}

QString I18nManager::language_enum_to_code(Language language) const {
  switch (language) {
    case Language::German: return "de";
    case Language::English: return "en";
    case Language::French: return "fr";
    case Language::Spanish: return "es";
    case Language::AutoDetect: return "auto";
    case Language::Count: return "count";
  }
  return "en";
}

bool I18nManager::load_translation_file(const QString& file_path) {
  QFileInfo fi(file_path);
  if (!fi.exists() || !fi.isFile()) {
    hh::get_logger().warn("I18nManager: Translation file not found: {}", qPrintable(file_path));
    return false;
  }
  
  main_translator_ = std::make_unique<QTranslator>();
  if (main_translator_->load(file_path)) {
    QCoreApplication::installTranslator(main_translator_.get());
    hh::get_logger().info("I18nManager: Translation loaded: {}", qPrintable(file_path));
    return true;
  }
  
  hh::get_logger().warn("I18nManager: Failed to load translation: {}", qPrintable(file_path));
  return false;
}

QString I18nManager::translation_file_path(const QString& language_code) const {
  if (translation_directory_.isEmpty()) {
    return QString(":%1/qthasihasi_%2.qm").arg(language_code);
  }
  return QString("%1/qthasihasi_%2.qm").arg(translation_directory_, language_code);
}

std::vector<QString> I18nManager::available_translations() const {
  std::vector<QString> translations;
  
  if (translation_directory_.isEmpty()) {
    return translations;
  }
  
  QDir dir(translation_directory_);
  QStringList filters;
  filters << "qthasihasi_*.qm";
  dir.setNameFilters(filters);
  dir.setFilter(QDir::Files);
  
  for (const QString& file : dir.entryList()) {
    translations.push_back(file.toStdString());
  }
  
  return translations;
}

} // namespace hh::client

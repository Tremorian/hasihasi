// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QColor>
#include <QMap>
#include <QFont>
#include <memory>

namespace hh::client {

// Theme-Manager für HasiHasi-Client
// Unterstützt Amber (Standard), Hell (Light Mode) und benutzerdefinierte Themes
class ThemeManager {
public:
  // Vordefinierte Themes
  enum class Theme {
    Amber,  // Standard (CRT-Style)
    Hell,   // Hell Mode
    Custom  // Benutzerdefiniert
  };

  // Theme-Konfiguration
  struct Config {
    // Farben
    QColor background;
    QColor foreground;
    QColor primary;
    QColor secondary;
    QColor accent;
    QColor error;
    QColor success;
    QColor warning;
    
    // Speaker-Farben (je nach Kanal)
    QColor speaker_color[8];
    
    // Schriftarten
    QFont font_family;
    int font_size;
    bool bold_headers;
    
    // UI-Parameter
    int border_radius;
    int padding;
    int spacing;
    
    // CRT-Effekt (nur Amber)
    bool crt_effect;
    float crt_brightness;
    
    // Standardwerte
    static Config default_amber();
    static Config default_hell();
  };

  ThemeManager() = default;
  ~ThemeManager() = default;

  // Theme setzen/laden
  bool set_theme(Theme theme);
  bool load_theme_from_file(const QString& file_path);
  bool save_theme_to_file(const QString& file_path) const;
  
  // Theme abrufen
  Theme current_theme() const;
  Config current_config() const;
  
  // Farbe abrufen
  QColor color(const QString& key) const;
  
  // QSS-Stylesheet generieren
  QString generate_stylesheet() const;
  
  // Theme-Datei verwalten
  static QStringList get_available_themes();
  static bool delete_theme(const QString& theme_name);
  
  // Callbacks
  using ThemeChangedCallback = std::function<void(Theme theme, const Config& config)>;
  void set_theme_changed_callback(ThemeChangedCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  
  Theme current_theme_;
  Config current_config_;
  ThemeChangedCallback theme_changed_callback_;
  
  void apply_config(const Config& config);
  QString generate_amber_stylesheet() const;
  QString generate_hell_stylesheet() const;
  QString generate_custom_stylesheet() const;
};

} // namespace hh::client

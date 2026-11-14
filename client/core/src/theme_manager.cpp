// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/theme_manager.hpp"
#include "hh/logging.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

namespace hh::client {

// ── Theme::Config ─────────────────────────────────────────────────────

ThemeManager::Config ThemeManager::Config::default_amber() {
  Config config;
  
  // Amber CRT-Style
  config.background = QColor(10, 10, 5);
  config.foreground = QColor(255, 191, 0);
  config.primary = QColor(255, 165, 0);
  config.secondary = QColor(200, 150, 0);
  config.accent = QColor(255, 215, 0);
  config.error = QColor(255, 50, 50);
  config.success = QColor(50, 255, 50);
  config.warning = QColor(255, 165, 0);
  
  // Speaker-Farben
  for (int i = 0; i < 8; ++i) {
    config.speaker_color[i] = QColor(255, 191 + i * 10, 0);
  }
  
  // Schriftarten
  config.font_family = QFont("Monospace", 10);
  config.font_size = 10;
  config.bold_headers = true;
  
  // UI-Parameter
  config.border_radius = 4;
  config.padding = 8;
  config.spacing = 4;
  
  // CRT-Effekt
  config.crt_effect = true;
  config.crt_brightness = 0.8f;
  
  return config;
}

ThemeManager::Config ThemeManager::Config::default_hell() {
  Config config;
  
  // Hell Mode
  config.background = QColor(245, 245, 245);
  config.foreground = QColor(30, 30, 30);
  config.primary = QColor(0, 100, 200);
  config.secondary = QColor(50, 120, 220);
  config.accent = QColor(0, 150, 255);
  config.error = QColor(220, 50, 50);
  config.success = QColor(50, 180, 50);
  config.warning = QColor(220, 180, 0);
  
  // Speaker-Farben
  for (int i = 0; i < 8; ++i) {
    config.speaker_color[i] = QColor(0, 100 + i * 20, 200);
  }
  
  // Schriftarten
  config.font_family = QFont("Sans Serif", 10);
  config.font_size = 10;
  config.bold_headers = true;
  
  // UI-Parameter
  config.border_radius = 6;
  config.padding = 8;
  config.spacing = 4;
  
  // CRT-Effekt
  config.crt_effect = false;
  config.crt_brightness = 1.0f;
  
  return config;
}

// ── ThemeManager-Implementierung ──────────────────────────────────────

struct ThemeManager::Impl {
  Config load_config_from_json(const QString& file_path) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
      return Config::default_amber();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    QJsonObject obj = doc.object();
    Config config;
    
    // Farben laden
    if (obj.contains("background")) {
      config.background = QColor(obj["background"].toString());
    }
    if (obj.contains("foreground")) {
      config.foreground = QColor(obj["foreground"].toString());
    }
    if (obj.contains("primary")) {
      config.primary = QColor(obj["primary"].toString());
    }
    if (obj.contains("secondary")) {
      config.secondary = QColor(obj["secondary"].toString());
    }
    if (obj.contains("accent")) {
      config.accent = QColor(obj["accent"].toString());
    }
    
    return config;
  }
  
  bool save_config_to_json(const QString& file_path, const Config& config) {
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly)) {
      return false;
    }
    
    QJsonObject obj;
    obj["background"] = config.background.name();
    obj["foreground"] = config.foreground.name();
    obj["primary"] = config.primary.name();
    obj["secondary"] = config.secondary.name();
    obj["accent"] = config.accent.name();
    
    QJsonDocument doc(obj);
    file.write(doc.toJson());
    file.close();
    
    return true;
  }
};

ThemeManager::Impl::Impl() = default;
ThemeManager::Impl::~Impl() = default;

// ── ThemeManager ──────────────────────────────────────────────────────

bool ThemeManager::set_theme(Theme theme) {
  Config config;
  
  switch (theme) {
    case Theme::Amber:
      config = Config::default_amber();
      break;
    case Theme::Hell:
      config = Config::default_hell();
      break;
    case Theme::Custom:
      // Benutzerdefiniertes Theme laden
      config = Config::default_amber(); // Fallback
      break;
  }
  
  apply_config(config);
  current_theme_ = theme;
  
  if (theme_changed_callback_) {
    theme_changed_callback_(theme, current_config_);
  }
  
  hh::get_logger().info("Theme: Set to {}", 
                        theme == Theme::Amber ? "Amber" : 
                        theme == Theme::Hell ? "Hell" : "Custom");
  
  return true;
}

bool ThemeManager::load_theme_from_file(const QString& file_path) {
  if (!impl_) {
    impl_ = std::make_unique<Impl>();
  }
  
  Config config = impl_->load_config_from_json(file_path);
  apply_config(config);
  current_theme_ = Theme::Custom;
  
  if (theme_changed_callback_) {
    theme_changed_callback_(current_theme_, current_config_);
  }
  
  hh::get_logger().info("Theme: Loaded from {}", file_path.toStdString());
  return true;
}

bool ThemeManager::save_theme_to_file(const QString& file_path) const {
  if (!impl_) {
    impl_ = std::make_unique<Impl>();
  }
  
  return impl_->save_config_to_json(file_path, current_config_);
}

ThemeManager::Theme ThemeManager::current_theme() const {
  return current_theme_;
}

ThemeManager::Config ThemeManager::current_config() const {
  return current_config_;
}

QColor ThemeManager::color(const QString& key) const {
  if (key == "background") return current_config_.background;
  if (key == "foreground") return current_config_.foreground;
  if (key == "primary") return current_config_.primary;
  if (key == "secondary") return current_config_.secondary;
  if (key == "accent") return current_config_.accent;
  if (key == "error") return current_config_.error;
  if (key == "success") return current_config_.success;
  if (key == "warning") return current_config_.warning;
  
  return QColor();
}

QString ThemeManager::generate_stylesheet() const {
  switch (current_theme_) {
    case Theme::Amber:
      return generate_amber_stylesheet();
    case Theme::Hell:
      return generate_hell_stylesheet();
    case Theme::Custom:
      return generate_custom_stylesheet();
  }
  return generate_amber_stylesheet();
}

QStringList ThemeManager::get_available_themes() {
  QStringList themes;
  themes << "Amber" << "Hell";
  
  // Benutzerdefinierte Themes laden
  QDir theme_dir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/themes/");
  if (theme_dir.exists()) {
    QDirIterator it(theme_dir, QStringList() << "*.json", QDir::Files);
    while (it.hasNext()) {
      it.next();
      themes << QFileInfo(it.filePath()).baseName();
    }
  }
  
  return themes;
}

bool ThemeManager::delete_theme(const QString& theme_name) {
  if (theme_name == "Amber" || theme_name == "Hell") {
    return false; // Vordefinierte Themes können nicht gelöscht werden
  }
  
  QDir theme_dir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/themes/");
  QString file_path = theme_dir.path() + "/" + theme_name + ".json";
  
  return QFile::remove(file_path);
}

void ThemeManager::set_theme_changed_callback(ThemeChangedCallback callback) {
  theme_changed_callback_ = std::move(callback);
}

void ThemeManager::apply_config(const Config& config) {
  current_config_ = config;
}

QString ThemeManager::generate_amber_stylesheet() const {
  return QString(
    "QWidget { "
    "background-color: %1; "
    "color: %2; "
    "font-family: %3; "
    "font-size: %4px; "
    "} "
    "QMainWindow { "
    "background-color: %1; "
    "} "
    "QMenuBar { "
    "background-color: %1; "
    "color: %2; "
    "} "
    "QMenuBar::item:selected { "
    "background-color: %3; "
    "} "
    "QMenu { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %3; "
    "} "
    "QMenu::item:selected { "
    "background-color: %3; "
    "} "
    "QPushButton { "
    "background-color: %3; "
    "color: %1; "
    "border: 1px solid %2; "
    "border-radius: %5px; "
    "padding: %6px; "
    "} "
    "QPushButton:hover { "
    "background-color: %4; "
    "} "
    "QPushButton:pressed { "
    "background-color: %2; "
    "} "
    "QLineEdit { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %3; "
    "border-radius: %5px; "
    "padding: %6px; "
    "} "
    "QTreeView { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %3; "
    "border-radius: %5px; "
    "} "
    "QTreeView::item:hover { "
    "background-color: %3; "
    "} "
    "QTabWidget::pane { "
    "border: 1px solid %3; "
    "border-radius: %5px; "
    "} "
    "QTabBar::tab { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %3; "
    "border-bottom-color: %1; "
    "border-top-left-radius: %5px; "
    "border-top-right-radius: %5px; "
    "padding: %6px; "
    "} "
    "QTabBar::tab:selected { "
    "background-color: %3; "
    "color: %1; "
    }"
  ).arg(current_config_.background.name())
   .arg(current_config_.foreground.name())
   .arg(current_config_.primary.name())
   .arg(current_config_.secondary.name())
   .arg(QString::number(current_config_.border_radius))
   .arg(QString::number(current_config_.padding));
}

QString ThemeManager::generate_hell_stylesheet() const {
  return QString(
    "QWidget { "
    "background-color: %1; "
    "color: %2; "
    "font-family: %3; "
    "font-size: %4px; "
    "} "
    "QMainWindow { "
    "background-color: %1; "
    "} "
    "QMenuBar { "
    "background-color: %1; "
    "color: %2; "
    "border-bottom: 1px solid %5; "
    "} "
    "QMenuBar::item:selected { "
    "background-color: %5; "
    "} "
    "QMenu { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %5; "
    "border-radius: 6px; "
    "} "
    "QMenu::item:selected { "
    "background-color: %5; "
    "} "
    "QPushButton { "
    "background-color: %5; "
    "color: %1; "
    "border: none; "
    "border-radius: 6px; "
    "padding: 8px 16px; "
    "font-weight: bold; "
    "} "
    "QPushButton:hover { "
    "background-color: %6; "
    "} "
    "QPushButton:pressed { "
    "background-color: %7; "
    "} "
    "QLineEdit { "
    "background-color: %8; "
    "color: %2; "
    "border: 1px solid %5; "
    "border-radius: 6px; "
    "padding: 8px; "
    "} "
    "QLineEdit:focus { "
    "border: 2px solid %5; "
    "} "
    "QTreeView { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %5; "
    "border-radius: 6px; "
    "padding: 4px; "
    "} "
    "QTreeView::item:hover { "
    "background-color: %8; "
    "} "
    "QTreeView::item:selected { "
    "background-color: %5; "
    "color: %1; "
    "} "
    "QTabWidget::pane { "
    "border: 1px solid %5; "
    "border-radius: 6px; "
    "} "
    "QTabBar::tab { "
    "background-color: %1; "
    "color: %2; "
    "border: 1px solid %5; "
    "border-bottom-color: %1; "
    "border-top-left-radius: 6px; "
    "border-top-right-radius: 6px; "
    "padding: 8px 16px; "
    "} "
    "QTabBar::tab:selected { "
    "background-color: %5; "
    "color: %1; "
    "font-weight: bold; "
    }"
  ).arg(current_config_.background.name())
   .arg(current_config_.foreground.name())
   .arg(current_config_.font_family.family())
   .arg(QString::number(current_config_.font_size))
   .arg(current_config_.primary.name())
   .arg(current_config_.secondary.name())
   .arg(current_config_.accent.name())
   .arg(current_config_.error.name());
}

QString ThemeManager::generate_custom_stylesheet() const {
  // Vorläufig dasselbe wie Amber
  return generate_amber_stylesheet();
}

} // namespace hh::client

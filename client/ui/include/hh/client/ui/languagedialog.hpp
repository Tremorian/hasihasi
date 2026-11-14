// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QDialog>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <memory>

namespace hh::client {
class I18nManager;
}

namespace hh::client::ui {

// Sprachwechsel-Dialog
class LanguageDialog : public QDialog {
  Q_OBJECT

public:
  explicit LanguageDialog(QWidget* parent = nullptr);
  ~LanguageDialog() override = default;

  // Nicht kopierbar, bewegbar
  LanguageDialog(const LanguageDialog&) = delete;
  LanguageDialog& operator=(const LanguageDialog&) = delete;
  LanguageDialog(LanguageDialog&&) noexcept;
  LanguageDialog& operator=(LanguageDialog&&) noexcept;

private slots:
  void on_language_changed(int index);
  void on_apply_clicked();
  void on_reset_clicked();
  void on_close_clicked();

private:
  // UI-Elemente initialisieren
  void setup_ui();
  
  // Spracheinstellungen laden
  void load_settings();
  
  // Spracheinstellungen speichern
  void save_settings();
  
  // UI-Elemente aktualisieren
  void update_ui();

  // UI-Elemente
  QComboBox* language_combo_;
  QPushButton* apply_button_;
  QPushButton* reset_button_;
  QPushButton* close_button_;
  QVBoxLayout* main_layout_;
  QHBoxLayout* button_layout_;
  QLabel* info_label_;

  // I18nManager
  I18nManager* i18n_manager_;
  
  // Settings
  QSettings* settings_;
};

} // namespace hh::client::ui

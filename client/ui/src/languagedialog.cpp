// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/ui/languagedialog.hpp"

#include "hh/client/i18n_manager.hpp"
#include "hh/logging.hpp"

#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QMessageBox>

namespace hh::client::ui {

// ── LanguageDialog ────────────────────────────────────────────────────

LanguageDialog::LanguageDialog(QWidget* parent)
  : QDialog(parent)
  , language_combo_(nullptr)
  , apply_button_(nullptr)
  , reset_button_(nullptr)
  , close_button_(nullptr)
  , main_layout_(nullptr)
  , button_layout_(nullptr)
  , info_label_(nullptr)
  , i18n_manager_(nullptr)
  , settings_(nullptr) {
  
  setup_ui();
  load_settings();
  update_ui();
  
  setWindowTitle(tr("Language"));
  setMinimumSize(400, 200);
}

LanguageDialog::LanguageDialog(LanguageDialog&& other) noexcept
  : QDialog(std::move(other))
  , language_combo_(std::move(other.language_combo_))
  , apply_button_(std::move(other.apply_button_))
  , reset_button_(std::move(other.reset_button_))
  , close_button_(std::move(other.close_button_))
  , main_layout_(std::move(other.main_layout_))
  , button_layout_(std::move(other.button_layout_))
  , info_label_(std::move(other.info_label_))
  , i18n_manager_(other.i18n_manager_)
  , settings_(other.settings_) {
  
  other.i18n_manager_ = nullptr;
  other.settings_ = nullptr;
}

LanguageDialog& LanguageDialog::operator=(LanguageDialog&& other) noexcept {
  if (this != &other) {
    QDialog::operator=(std::move(other));
    language_combo_ = std::move(other.language_combo_);
    apply_button_ = std::move(other.apply_button_);
    reset_button_ = std::move(other.reset_button_);
    close_button_ = std::move(other.close_button_);
    main_layout_ = std::move(other.main_layout_);
    button_layout_ = std::move(other.button_layout_);
    info_label_ = std::move(other.info_label_);
    i18n_manager_ = other.i18n_manager_;
    settings_ = other.settings_;
    other.i18n_manager_ = nullptr;
    other.settings_ = nullptr;
  }
  return *this;
}

void LanguageDialog::setup_ui() {
  // Hauptlayout
  main_layout_ = new QVBoxLayout(this);
  
  // Sprachauswahl
  language_combo_ = new QComboBox();
  language_combo_->addItem("Deutsch", static_cast<int>(I18nManager::Language::German));
  language_combo_->addItem("English", static_cast<int>(I18nManager::Language::English));
  language_combo_->addItem("Français", static_cast<int>(I18nManager::Language::French));
  language_combo_->addItem("Español", static_cast<int>(I18nManager::Language::Spanish));
  language_combo_->addItem(tr("Auto-detect"), static_cast<int>(I18nManager::Language::AutoDetect));
  
  main_layout_->addWidget(new QLabel(tr("Select language:")));
  main_layout_->addWidget(language_combo_);
  
  // Info-Label
  info_label_ = new QLabel(tr("Changes will be applied after restart."));
  info_label_->setStyleSheet("color: gray; font-style: italic;");
  main_layout_->addWidget(info_label_);
  
  // Buttons
  button_layout_ = new QHBoxLayout();
  
  apply_button_ = new QPushButton(tr("Apply"));
  reset_button_ = new QPushButton(tr("Reset"));
  close_button_ = new QPushButton(tr("Close"));
  
  button_layout_->addWidget(apply_button_);
  button_layout_->addWidget(reset_button_);
  button_layout_->addStretch();
  button_layout_->addWidget(close_button_);
  
  main_layout_->addLayout(button_layout_);
  
  // Connections
  connect(language_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &LanguageDialog::on_language_changed);
  connect(apply_button_, &QPushButton::clicked, this, &LanguageDialog::on_apply_clicked);
  connect(reset_button_, &QPushButton::clicked, this, &LanguageDialog::on_reset_clicked);
  connect(close_button_, &QPushButton::clicked, this, &LanguageDialog::on_close_clicked);
}

void LanguageDialog::load_settings() {
  // I18nManager und Settings initialisieren
  if (!i18n_manager_) {
    // I18nManager aus dem globalen Kontext holen (oder neu erstellen)
    i18n_manager_ = new I18nManager(this);
  }
  
  if (!settings_) {
    settings_ = new QSettings("HasiHasi", "HasiHasi", this);
  }
  
  // Spracheinstellungen laden
  QString language_key = "language";
  if (settings_->contains(language_key)) {
    int language_code = settings_->value(language_key).toInt();
    for (int i = 0; i < language_combo_->count(); ++i) {
      if (language_combo_->itemData(i).toInt() == language_code) {
        language_combo_->setCurrentIndex(i);
        break;
      }
    }
  }
}

void LanguageDialog::save_settings() {
  if (!settings_) {
    return;
  }
  
  // Spracheinstellungen speichern
  int language_code = language_combo_->currentData().toInt();
  settings_->setValue("language", language_code);
}

void LanguageDialog::update_ui() {
  // UI-Elemente aktualisieren
  if (i18n_manager_) {
    I18nManager::Language current_lang = i18n_manager_->current_language();
    for (int i = 0; i < language_combo_->count(); ++i) {
      if (language_combo_->itemData(i).toInt() == static_cast<int>(current_lang)) {
        language_combo_->setCurrentIndex(i);
        break;
      }
    }
  }
}

void LanguageDialog::on_language_changed(int index) {
  Q_UNUSED(index);
  // Sprache wurde geändert, aber noch nicht angewendet
}

void LanguageDialog::on_apply_clicked() {
  // Spracheinstellungen speichern
  save_settings();
  
  // I18nManager aktualisieren
  if (i18n_manager_) {
    int language_code = language_combo_->currentData().toInt();
    I18nManager::Language language = static_cast<I18nManager::Language>(language_code);
    i18n_manager_->set_language(language);
  }
  
  // Info anzeigen
  QMessageBox::information(this, tr("Language Changed"),
                           tr("The language has been changed. Please restart the application for the changes to take effect."));
  
  // Dialog schließen
  accept();
}

void LanguageDialog::on_reset_clicked() {
  // Sprache auf Auto-detect zurücksetzen
  language_combo_->setCurrentIndex(language_combo_->count() - 1);
  
  // Spracheinstellungen speichern
  save_settings();
  
  // I18nManager aktualisieren
  if (i18n_manager_) {
    i18n_manager_->set_language(I18nManager::Language::AutoDetect);
  }
}

void LanguageDialog::on_close_clicked() {
  reject();
}

} // namespace hh::client::ui

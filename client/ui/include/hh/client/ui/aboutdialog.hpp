// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>

namespace hh::client {

// "Über HasiHasi"-Dialog
class AboutDialog : public QDialog {
  Q_OBJECT

public:
  explicit AboutDialog(QWidget *parent = nullptr);
  ~AboutDialog() override = default;

private slots:
  void on_license_clicked();
  void on_github_clicked();
  void on_close_clicked();

private:
  void setup_ui();
  void setup_about_section();
  void setup_license_section();
  void setup_credits_section();

  QLabel *logo_label_;
  QLabel *title_label_;
  QLabel *version_label_;
  QLabel *description_label_;
  QTextBrowser *license_browser_;
  QPushButton *license_btn_;
  QPushButton *github_btn_;
  QPushButton *close_btn_;
};

} // namespace hh::client

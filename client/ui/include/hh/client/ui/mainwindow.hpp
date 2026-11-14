// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QMainWindow>
#include <QLabel>

namespace hh::client::ui {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override = default;

private:
  void setup_ui();
  void apply_amber_theme();

  QLabel* status_label_ = nullptr;
};

} // namespace hh::client::ui

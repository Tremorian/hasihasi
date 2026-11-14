// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/ui/mainwindow.hpp"

namespace hh::client::ui {

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent) {
  setup_ui();
  apply_amber_theme();
}

void MainWindow::setup_ui() {
  setWindowTitle("HasiHasi");
  setMinimumSize(400, 300);

  auto* central = new QWidget(this);
  setCentralWidget(central);

  status_label_ = new QLabel("Verbinden...", central);
  status_label_->setGeometry(20, 20, 360, 30);
}

void MainWindow::apply_amber_theme() {
  // Amber-CRT-Theme: bernsteinfarbener Phosphor auf Schwarz
  setStyleSheet(R"(
    QMainWindow {
      background-color: #0A0A08;
    }
    QLabel {
      color: #FFB000;
      font-family: 'Inter', sans-serif;
    }
  )");
}

} // namespace hh::client::ui

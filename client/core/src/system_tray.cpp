// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/system_tray.hpp"

#include "hh/logging.hpp"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QMessageBox>

namespace hh::client {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct SystemTray::Impl {
  QSystemTrayIcon* tray_icon = nullptr;
  QMenu* context_menu = nullptr;

  QAction* action_connect = nullptr;
  QAction* action_disconnect = nullptr;
  QAction* action_mute = nullptr;
  QAction* action_settings = nullptr;
  QAction* action_quit = nullptr;

  bool connected = false;
  std::string status = "Disconnected";

  Impl() {
    tray_icon = new QSystemTrayIcon();
    context_menu = new QMenu();

    // Menü-Actions erstellen
    action_connect = new QAction("Verbinden", context_menu);
    action_disconnect = new QAction("Trennen", context_menu);
    action_mute = new QAction("Stummschalten", context_menu);
    action_settings = new QAction("Einstellungen", context_menu);
    action_quit = new QAction("Beenden", context_menu);

    // Actions zum Menü hinzufügen
    context_menu->addAction(action_connect);
    context_menu->addAction(action_disconnect);
    context_menu->addAction(action_mute);
    context_menu->addAction(action_settings);
    context_menu->addSeparator();
    context_menu->addAction(action_quit);

    // Tray-Icon setzen
    tray_icon->setContextMenu(context_menu);
    tray_icon->setIcon(QIcon::fromTheme("audio-input-microphone", QIcon(":/logo_placeholder.svg")));
    tray_icon->show();

    // Tooltips setzen
    tray_icon->setToolTip("HasiHasi - Voice Chat");
  }

  ~Impl() {
    if (tray_icon) {
      tray_icon->hide();
      delete tray_icon;
    }
    if (context_menu) {
      delete context_menu;
    }
  }
};

// ── SystemTray ────────────────────────────────────────────────────────

bool SystemTray::initialize() {
  if (is_initialized_) {
    return true;
  }

  impl_ = std::make_unique<Impl>();
  is_initialized_ = true;

  hh::get_logger().info("SystemTray: Initialized");
  return true;
}

void SystemTray::shutdown() {
  if (!is_initialized_) {
    return;
  }

  impl_.reset();
  is_initialized_ = false;

  hh::get_logger().info("SystemTray: Shutdown");
}

void SystemTray::set_action_connect(const std::function<void()>& callback) {
  if (impl_ && impl_->action_connect) {
    QObject::connect(impl_->action_connect, &QAction::triggered, callback);
  }
}

void SystemTray::set_action_disconnect(const std::function<void()>& callback) {
  if (impl_ && impl_->action_disconnect) {
    QObject::connect(impl_->action_disconnect, &QAction::triggered, callback);
  }
}

void SystemTray::set_action_mute(const std::function<void()>& callback) {
  if (impl_ && impl_->action_mute) {
    QObject::connect(impl_->action_mute, &QAction::triggered, callback);
  }
}

void SystemTray::set_action_settings(const std::function<void()>& callback) {
  if (impl_ && impl_->action_settings) {
    QObject::connect(impl_->action_settings, &QAction::triggered, callback);
  }
}

void SystemTray::set_action_quit(const std::function<void()>& callback) {
  if (impl_ && impl_->action_quit) {
    QObject::connect(impl_->action_quit, &QAction::triggered, callback);
  }
}

void SystemTray::set_status(const std::string& status) {
  if (impl_) {
    impl_->status = status;
    impl_->tray_icon->setToolTip("HasiHasi - " + QString::fromStdString(status));
  }
}

void SystemTray::set_connected(bool connected) {
  if (impl_) {
    impl_->connected = connected;
    impl_->action_connect->setVisible(!connected);
    impl_->action_disconnect->setVisible(connected);

    std::string new_status = connected ? "Connected" : "Disconnected";
    set_status(new_status);
  }
}

void SystemTray::show() {
  if (impl_) {
    impl_->tray_icon->show();
  }
}

void SystemTray::hide() {
  if (impl_) {
    impl_->tray_icon->hide();
  }
}

void SystemTray::show_message(const std::string& title, const std::string& message) {
  if (impl_) {
    QMessageBox::information(nullptr,
                             QString::fromStdString(title),
                             QString::fromStdString(message));
  }
}

} // namespace hh::client

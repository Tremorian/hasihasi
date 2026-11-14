// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/dbus_interface.hpp"
#include "hh/logging.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

namespace hh::client {

// ── HasiHasiDBus (Daemon) ────────────────────────────────────────────

HasiHasiDBus::HasiHasiDBus(QObject *parent)
  : QDBusAbstractAdaptor(parent) {
  // Auto-registration
  QDBusConnection::sessionBus().registerService("de.hasihasi.hasihasi");
  QDBusConnection::sessionBus().registerObject("/de/hasihasi/hasihasi", this);
  
  hh::get_logger().info("DBus: HasiHasi daemon registered on session bus");
}

bool HasiHasiDBus::connectToServer(const QString &server_address, int port) {
  hh::get_logger().info("DBus: Connect to server {}:{}" , server_address, port);
  // Implementierung in der Client-Logik
  emit connectionChanged(true);
  emit serverChanged(server_address);
  return true;
}

bool HasiHasiDBus::disconnectFromServer() {
  hh::get_logger().info("DBus: Disconnect from server");
  emit connectionChanged(false);
  return true;
}

bool HasiHasiDBus::isConnected() const {
  return true; // Mock
}

uint HasiHasiDBus::joinChannel(const QString &channel_name) {
  hh::get_logger().info("DBus: Join channel {}", channel_name.toStdString());
  return 1; // Mock channel_id
}

bool HasiHasiDBus::leaveChannel(uint channel_id) {
  hh::get_logger().info("DBus: Leave channel {}", channel_id);
  return true;
}

QList<QMap<QString, QVariant>> HasiHasiDBus::getChannels() const {
  QList<QMap<QString, QVariant>> channels;
  // Mock
  return channels;
}

bool HasiHasiDBus::muteUser(uint user_id, bool muted) {
  hh::get_logger().info("DBus: Mute user {} = {}", user_id, muted);
  return true;
}

bool HasiHasiDBus::deafenUser(uint user_id, bool deafened) {
  hh::get_logger().info("DBus: Deafen user {} = {}", user_id, deafened);
  return true;
}

bool HasiHasiDBus::setVolume(uint user_id, int volume) {
  hh::get_logger().info("DBus: Set volume for user {} to {}", user_id, volume);
  return true;
}

bool HasiHasiDBus::sendChannelMessage(uint channel_id, const QString &message) {
  hh::get_logger().info("DBus: Send channel message to {}", channel_id);
  return true;
}

bool HasiHasiDBus::sendPrivateMessage(uint user_id, const QString &message) {
  hh::get_logger().info("DBus: Send private message to {}", user_id);
  return true;
}

bool HasiHasiDBus::setPTT(bool active) {
  hh::get_logger().info("DBus: PTT = {}", active);
  return true;
}

bool HasiHasiDBus::isPTTActive() const {
  return false; // Mock
}

bool HasiHasiDBus::setVolume(int volume) {
  hh::get_logger().info("DBus: Set master volume to {}", volume);
  return true;
}

int HasiHasiDBus::volume() const {
  return 50; // Mock
}

bool HasiHasiDBus::setMute(bool muted) {
  hh::get_logger().info("DBus: Mute = {}", muted);
  return true;
}

bool HasiHasiDBus::isMuted() const {
  return false; // Mock
}

QString HasiHasiDBus::serverAddress() const {
  return "localhost"; // Mock
}

int HasiHasiDBus::serverPort() const {
  return 64738; // Mock
}

QString HasiHasiDBus::username() const {
  return "TestUser"; // Mock
}

uint HasiHasiDBus::userId() const {
  return 1; // Mock
}

// ── DBusClient ────────────────────────────────────────────────────────

DBusClient::DBusClient(QObject *parent)
  : QObject(parent)
  , dbus_interface_(nullptr)
  , is_connected_(false) {
}

bool DBusClient::connectToDaemon() {
  dbus_interface_ = new QDBusInterface(
    "de.hasihasi.hasihasi",
    "/de/hasihasi/hasihasi",
    "de.hasihasi.hasihasi",
    QDBusConnection::sessionBus(),
    this
  );

  if (!dbus_interface_->isValid()) {
    hh::get_logger().error("DBus: Failed to connect to daemon: {}", 
                           dbus_interface_->error().message().toStdString());
    return false;
  }

  // Signale verbinden
  QDBusConnection::sessionBus().connect(
    "de.hasihasi.hasihasi",
    "/de/hasihasi/hasihasi",
    "de.hasihasi.hasihasi",
    "speakerChanged",
    this,
    SLOT(on_speaker_changed(uint, QString, bool))
  );

  QDBusConnection::sessionBus().connect(
    "de.hasihasi.hasihasi",
    "/de/hasihasi/hasihasi",
    "de.hasihasi.hasihasi",
    "textMessageReceived",
    this,
    SLOT(on_text_message_received(uint, QString, QString, uint))
  );

  QDBusConnection::sessionBus().connect(
    "de.hasihasi.hasihasi",
    "/de/hasihasi/hasihasi",
    "de.hasihasi.hasihasi",
    "notificationReceived",
    this,
    SLOT(on_notification_received(QString, QString, uint))
  );

  is_connected_ = true;
  hh::get_logger().info("DBus: Connected to daemon");
  return true;
}

void DBusClient::disconnectFromDaemon() {
  if (dbus_interface_) {
    delete dbus_interface_;
    dbus_interface_ = nullptr;
  }
  is_connected_ = false;
  hh::get_logger().info("DBus: Disconnected from daemon");
}

bool DBusClient::isConnected() const {
  return is_connected_;
}

void DBusClient::set_speaker_callback(SpeakerCallback callback) {
  speaker_callback_ = std::move(callback);
}

void DBusClient::set_message_callback(MessageCallback callback) {
  message_callback_ = std::move(callback);
}

void DBusClient::set_notification_callback(NotificationCallback callback) {
  notification_callback_ = std::move(callback);
}

void DBusClient::on_speaker_changed(uint user_id, const QString& username, bool is_speaking) {
  if (speaker_callback_) {
    speaker_callback_(user_id, username, is_speaking);
  }
}

void DBusClient::on_text_message_received(uint user_id, const QString& sender_name, const QString& content, uint channel_id) {
  if (message_callback_) {
    message_callback_(user_id, sender_name.toStdString(), content.toStdString(), channel_id);
  }
}

void DBusClient::on_notification_received(const QString& title, const QString& message, uint priority) {
  if (notification_callback_) {
    notification_callback_(title.toStdString(), message.toStdString(), priority);
  }
}

} // namespace hh::client

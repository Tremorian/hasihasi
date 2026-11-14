// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <vector>
#include <string>
#include <map>

namespace hh::client {

// D-Bus-Interface für HasiHasi-Client
// Interface: de.hasihasi.hasihasi
// Pfad: /de/hasihasi/hasihasi
class HasiHasiDBus : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "de.hasihasi.hasihasi")

public:
  explicit HasiHasiDBus(QObject *parent);
  ~HasiHasiDBus() override = default;

signals:
  // Sprecher-Status
  void speakerChanged(uint user_id, const QString &username, bool is_speaking);
  
  // Textnachrichten
  void textMessageReceived(uint user_id, const QString &sender_name, const QString &content, uint channel_id);
  void privateMessageReceived(uint user_id, const QString &sender_name, const QString &content);
  
  // Verbindungsstatus
  void connectionChanged(bool connected);
  void serverChanged(const QString &server_address);
  
  // Kanal-Status
  void channelUsersChanged(uint channel_id, const QList<uint> &user_ids);
  
  // Benachrichtigungen
  void notificationReceived(const QString &title, const QString &message, uint priority);

public slots:
  // Verbindungs-Management
  bool connectToServer(const QString &server_address, int port = 64738);
  bool disconnectFromServer();
  bool isConnected() const;
  
  // Kanal-Management
  uint joinChannel(const QString &channel_name);
  bool leaveChannel(uint channel_id);
  QList<QMap<QString, QVariant>> getChannels() const;
  
  // Nutzer-Management
  bool muteUser(uint user_id, bool muted);
  bool deafenUser(uint user_id, bool deafened);
  bool setVolume(uint user_id, int volume); // 0-100
  
  // Textchat
  bool sendChannelMessage(uint channel_id, const QString &message);
  bool sendPrivateMessage(uint user_id, const QString &message);
  
  // PTT (Push-to-Talk)
  bool setPTT(bool active);
  bool isPTTActive() const;
  
  // Einstellungen
  bool setVolume(int volume); // Master-Volumen 0-100
  int volume() const;
  bool setMute(bool muted);
  bool isMuted() const;
  
  // Server-Info
  QString serverAddress() const;
  int serverPort() const;
  QString username() const;
  uint userId() const;
};

// D-Bus-Client für externe Steuerung
class DBusClient : public QObject {
  Q_OBJECT

public:
  explicit DBusClient(QObject *parent = nullptr);
  ~DBusClient() override = default;

  // Verbindung zum Daemon
  bool connectToDaemon();
  void disconnectFromDaemon();
  bool isConnected() const;

  // Signale empfangen
  using SpeakerCallback = std::function<void(uint, const QString&, bool)>;
  using MessageCallback = std::function<void(uint, const QString&, const QString&, uint)>;
  using NotificationCallback = std::function<void(const QString&, const QString&, uint)>;

  void set_speaker_callback(SpeakerCallback callback);
  void set_message_callback(MessageCallback callback);
  void set_notification_callback(NotificationCallback callback);

private slots:
  void on_speaker_changed(uint user_id, const QString& username, bool is_speaking);
  void on_text_message_received(uint user_id, const QString& sender_name, const QString& content, uint channel_id);
  void on_notification_received(const QString& title, const QString& message, uint priority);

private:
  QDBusInterface* dbus_interface_;
  bool is_connected_;
  
  SpeakerCallback speaker_callback_;
  MessageCallback message_callback_;
  NotificationCallback notification_callback_;
};

} // namespace hh::client

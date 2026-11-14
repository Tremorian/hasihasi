// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

namespace hh::client {

// Push-Benachrichtigungen für Mobile-Client
class PushNotificationManager : public QObject {
  Q_OBJECT

public:
  explicit PushNotificationManager(QObject* parent = nullptr);
  ~PushNotificationManager() override = default;

  // Nicht kopierbar, bewegbar
  PushNotificationManager(const PushNotificationManager&) = delete;
  PushNotificationManager& operator=(const PushNotificationManager&) = delete;
  PushNotificationManager(PushNotificationManager&&) noexcept;
  PushNotificationManager& operator=(PushNotificationManager&&) noexcept;

  // Plattform-Erkennung
  enum class Platform {
    ANDROID,
    IOS,
    DESKTOP
  };
  Q_ENUM(Platform)

  // Plattform ermitteln
  Platform platform() const;

  // Push-Benachrichtigungen registrieren
  bool register_for_push(const QString& device_token);
  bool unregister_for_push();

  // Token abrufen
  QString get_device_token() const;

  // Status
  enum class Status {
    NOT_INITIALIZED,
    INITIALIZED,
    REGISTERED,
    ERROR
  };
  Q_ENUM(Status)

  Status status() const;

  // Konfiguration
  void set_fcm_server_key(const QString& key);
  void set_apns_certificate(const QString& cert_path);
  void set_server_url(const QString& url);

  // Callbacks
  using PushCallback = std::function<void(const QString& type, const QString& data)>;
  void set_push_callback(PushCallback callback);

private slots:
  void on_push_received(const QString& type, const QString& data);
  void on_registration_complete(const QString& token);
  void on_registration_error(const QString& error);

signals:
  void status_changed(Status status);
  void push_received(const QString& type, const QString& data);
  void error_occurred(const QString& error);

private:
  Platform platform_;
  Status status_ = Status::NOT_INITIALIZED;
  QString device_token_;
  QString fcm_server_key_;
  QString apns_certificate_;
  QString server_url_;
  PushCallback push_callback_;
};

} // namespace hh::client

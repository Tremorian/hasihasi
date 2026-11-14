// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "push_notification_manager.hpp"

#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QSettings>

namespace hh::client {

PushNotificationManager::PushNotificationManager(QObject* parent)
  : QObject(parent)
  , platform_(Platform::DESKTOP) {
  
  // Plattform ermitteln
  QOperatingSystemVersion os = QOperatingSystemVersion::current();
  
  if (os.type() == QOperatingSystemVersion::OS_Android) {
    platform_ = Platform::ANDROID;
  } else if (os.type() == QOperatingSystemVersion::OS_IOS) {
    platform_ = Platform::IOS;
  } else {
    platform_ = Platform::DESKTOP;
  }
  
  // Status setzen
  status_ = Status::INITIALIZED;
  emit status_changed(status_);
}

PushNotificationManager::PushNotificationManager(PushNotificationManager&& other) noexcept
  : QObject(other.parent())
  , platform_(other.platform_)
  , status_(other.status_)
  , device_token_(std::move(other.device_token_))
  , fcm_server_key_(std::move(other.fcm_server_key_))
  , apns_certificate_(std::move(other.apns_certificate_))
  , server_url_(std::move(other.server_url_))
  , push_callback_(std::move(other.push_callback_)) {
  
  other.platform_ = Platform::DESKTOP;
  other.status_ = Status::NOT_INITIALIZED;
  other.device_token_.clear();
  other.fcm_server_key_.clear();
  other.apns_certificate_.clear();
  other.server_url_.clear();
}

PushNotificationManager& PushNotificationManager::operator=(PushNotificationManager&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    platform_ = other.platform_;
    status_ = other.status_;
    device_token_ = std::move(other.device_token_);
    fcm_server_key_ = std::move(other.fcm_server_key_);
    apns_certificate_ = std::move(other.apns_certificate_);
    server_url_ = std::move(other.server_url_);
    push_callback_ = std::move(other.push_callback_);
    other.platform_ = Platform::DESKTOP;
    other.status_ = Status::NOT_INITIALIZED;
    other.device_token_.clear();
    other.fcm_server_key_.clear();
    other.apns_certificate_.clear();
    other.server_url_.clear();
  }
  return *this;
}

// ── Plattform ermitteln ──────────────────────────────────────────────

PushNotificationManager::Platform PushNotificationManager::platform() const {
  return platform_;
}

// ── Push-Benachrichtigungen registrieren ─────────────────────────────

bool PushNotificationManager::register_for_push(const QString& device_token) {
  if (device_token.isEmpty()) {
    emit error_occurred("Geräte-Token ist leer");
    status_ = Status::ERROR;
    emit status_changed(status_);
    return false;
  }
  
  device_token_ = device_token;
  
  // Token an Server senden
  if (!server_url_.isEmpty()) {
    QNetworkAccessManager manager;
    QUrl url(server_url_ + "/api/push/register");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["device_token"] = device_token_;
    json["platform"] = (platform_ == Platform::ANDROID) ? "android" : "ios";
    
    QNetworkReply* reply = manager.post(request, QJsonDocument(json).toJson());
    
    // In einer echten Implementierung würde hier auf die Antwort gewartet werden
    // Für jetzt setzen wir den Status direkt auf REGISTERED
    status_ = Status::REGISTERED;
    emit status_changed(status_);
    
    reply->deleteLater();
    return true;
  }
  
  status_ = Status::REGISTERED;
  emit status_changed(status_);
  return true;
}

bool PushNotificationManager::unregister_for_push() {
  if (device_token_.isEmpty()) {
    return false;
  }
  
  // Token vom Server entfernen
  if (!server_url_.isEmpty()) {
    QNetworkAccessManager manager;
    QUrl url(server_url_ + "/api/push/unregister");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["device_token"] = device_token_;
    
    QNetworkReply* reply = manager.post(request, QJsonDocument(json).toJson());
    reply->deleteLater();
  }
  
  device_token_.clear();
  status_ = Status::INITIALIZED;
  emit status_changed(status_);
  return true;
}

// ── Token abrufen ────────────────────────────────────────────────────

QString PushNotificationManager::get_device_token() const {
  return device_token_;
}

// ── Status ───────────────────────────────────────────────────────────

PushNotificationManager::Status PushNotificationManager::status() const {
  return status_;
}

// ── Konfiguration ────────────────────────────────────────────────────

void PushNotificationManager::set_fcm_server_key(const QString& key) {
  fcm_server_key_ = key;
}

void PushNotificationManager::set_apns_certificate(const QString& cert_path) {
  apns_certificate_ = cert_path;
}

void PushNotificationManager::set_server_url(const QString& url) {
  server_url_ = url;
}

// ── Callbacks ────────────────────────────────────────────────────────

void PushNotificationManager::set_push_callback(PushCallback callback) {
  push_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void PushNotificationManager::on_push_received(const QString& type, const QString& data) {
  emit push_received(type, data);
  
  if (push_callback_) {
    push_callback_(type, data);
  }
}

void PushNotificationManager::on_registration_complete(const QString& token) {
  device_token_ = token;
  status_ = Status::REGISTERED;
  emit status_changed(status_);
}

void PushNotificationManager::on_registration_error(const QString& error) {
  emit error_occurred(error);
  status_ = Status::ERROR;
  emit status_changed(status_);
}

} // namespace hh::client

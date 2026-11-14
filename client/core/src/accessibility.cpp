// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/accessibility.hpp"
#include "hh/logging.hpp"

#include <QWidget>
#include <QAccessible>
#include <QAccessibleInterface>
#include <QAccessibleValueInterface>
#include <QAccessibleTextInterface>
#include <QAccessibleStateInterface>
#include <QAccessibleActionInterface>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace hh::client {

// ── HasiHasiAccessibility ─────────────────────────────────────────────

HasiHasiAccessibility::HasiHasiAccessibility(QObject *parent)
  : QObject(parent)
  , enabled_(true) {
  
  setup_custom_roles();
  
  hh::get_logger().info("Accessibility: Initialized (enabled: {})", enabled_);
}

void HasiHasiAccessibility::enable_accessibility(bool enabled) {
  enabled_ = enabled;
  hh::get_logger().info("Accessibility: {}", enabled ? "enabled" : "disabled");
}

void HasiHasiAccessibility::register_widget(QWidget *widget, const QString &role, const QString &text) {
  if (!widget || !enabled_) return;
  
  registered_widgets_[widget] = static_cast<QAccessible::Role>(role.toInt());
  
  // Widget accessible name setzen
  widget->setAccessibleName(text);
  widget->setAccessibleDescription("HasiHasi " + role);
  
  hh::get_logger().debug("Accessibility: Registered widget {} with role {}", 
                        widget->objectName().toStdString(), role.toStdString());
}

void HasiHasiAccessibility::unregister_widget(QWidget *widget) {
  if (!widget) return;
  
  registered_widgets_.erase(widget);
  
  hh::get_logger().debug("Accessibility: Unregistered widget {}", widget->objectName().toStdString());
}

void HasiHasiAccessibility::update_speaker_status(uint user_id, const QString &username, bool is_speaking) {
  if (!enabled_) return;
  
  QString message = QString("%1 %2").arg(is_speaking ? "Spricht" : "Hört auf zu sprechen")
                                     .arg(username);
  announce(message);
}

void HasiHasiAccessibility::update_connection_status(bool connected, const QString &server_address) {
  if (!enabled_) return;
  
  QString message = QString("Verbunden mit %1").arg(server_address);
  if (!connected) {
    message = "Verbindung getrennt";
  }
  announce(message);
}

void HasiHasiAccessibility::update_channel_status(uint channel_id, const QString &channel_name, uint user_count) {
  if (!enabled_) return;
  
  QString message = QString("Kanal %1 mit %2 Nutzern").arg(channel_name).arg(user_count);
  announce(message);
}

void HasiHasiAccessibility::announce(const QString &message, QAccessible::Event event) {
  if (!enabled_) return;
  
  hh::get_logger().info("Accessibility: Announce: {}", message.toStdString());
  
  // Qt Accessibility Event auslösen
  QAccessibleEvent accessible_event(QAccessible::instance(), event, const_cast<QString*>(&message));
  QCoreApplication::sendEvent(QCoreApplication::instance(), &accessible_event);
}

void HasiHasiAccessibility::setup_custom_roles() {
  // Custom roles registrieren
  QAccessible::installFactory([](QObject *object, const QVariantVector &parameters) -> QAccessibleInterface * {
    if (!object) return nullptr;
    
    // Hier könnten custom accessibility interfaces installiert werden
    return nullptr;
  });
}

// ── ChannelTreeAccessibility ──────────────────────────────────────────

ChannelTreeAccessibility::ChannelTreeAccessibility(QWidget *widget)
  : QAccessibleInterface(widget)
  , widget_(widget)
  , channel_name_("")
  , user_count_(0)
  , is_expanded_(false) {
}

QAccessible::InterfaceType ChannelTreeAccessibility::type() const {
  return QAccessible::List;
}

QAccessible::Role ChannelTreeAccessibility::role() const {
  return QAccessible::List;
}

QAccessible::State ChannelTreeAccessibility::state() const {
  QAccessible::State state;
  state.expanded = is_expanded_;
  return state;
}

QObject *ChannelTreeAccessibility::object() const {
  return widget_;
}

QAccessibleInterface *ChannelTreeAccessibility::child(QAccessible::ChildId id) const {
  (void)id;
  return nullptr;
}

QAccessibleInterface *ChannelTreeAccessibility::child(const QPoint &pos) const {
  (void)pos;
  return nullptr;
}

int ChannelTreeAccessibility::childCount() const {
  return 0;
}

int ChannelTreeAccessibility::indexOfChild(const QAccessibleInterface *child) const {
  (void)child;
  return -1;
}

QString ChannelTreeAccessibility::text(QAccessible::Text t, QAccessible::Relation relation,
                                        int relatedToIndex) const {
  (void)relation;
  (void)relatedToIndex;
  
  switch (t) {
    case QAccessible::Name:
      return channel_name_;
    case QAccessible::Description:
      return QString("%1 Nutzer").arg(user_count_);
    default:
      return QString();
  }
}

QVariant ChannelTreeAccessibility::value() const {
  return user_count_;
}

QVariant ChannelTreeAccessibility::actionText(const QString &actionName) const {
  if (actionName == "expand") return "Aufklappen";
  if (actionName == "collapse") return "Zuklappen";
  if (actionName == "select") return "Auswählen";
  return QString();
}

bool ChannelTreeAccessibility::doAction(const QString &actionName) {
  if (actionName == "expand") {
    is_expanded_ = true;
    return true;
  }
  if (actionName == "collapse") {
    is_expanded_ = false;
    return true;
  }
  if (actionName == "select") {
    // Kanal auswählen
    return true;
  }
  return false;
}

// ── VoiceButtonAccessibility ──────────────────────────────────────────

VoiceButtonAccessibility::VoiceButtonAccessibility(QWidget *widget)
  : QAccessibleInterface(widget)
  , widget_(widget)
  , is_muted_(false)
  , is_deafened_(false)
  , volume_(50) {
}

QAccessible::InterfaceType VoiceButtonAccessibility::type() const {
  return QAccessible::Button;
}

QAccessible::Role VoiceButtonAccessibility::role() const {
  return QAccessible::PushButton;
}

QAccessible::State VoiceButtonAccessibility::state() const {
  QAccessible::State state;
  state.focusable = true;
  state.focusable = true;
  state.checkable = true;
  state.checked = is_muted_;
  return state;
}

QObject *VoiceButtonAccessibility::object() const {
  return widget_;
}

QAccessibleInterface *VoiceButtonAccessibility::child(QAccessible::ChildId id) const {
  (void)id;
  return nullptr;
}

QAccessibleInterface *VoiceButtonAccessibility::child(const QPoint &pos) const {
  (void)pos;
  return nullptr;
}

int VoiceButtonAccessibility::childCount() const {
  return 0;
}

int VoiceButtonAccessibility::indexOfChild(const QAccessibleInterface *child) const {
  (void)child;
  return -1;
}

QString VoiceButtonAccessibility::text(QAccessible::Text t, QAccessible::Relation relation,
                                        int relatedToIndex) const {
  (void)relation;
  (void)relatedToIndex;
  
  switch (t) {
    case QAccessible::Name:
      if (is_muted_) return "Stumm";
      if (is_deafened_) return "Taub";
      return "Stummschaltung";
    case QAccessible::Description:
      return QString("Volumen: %1%").arg(volume_);
    default:
      return QString();
  }
}

QVariant VoiceButtonAccessibility::value() const {
  return volume_;
}

QVariant VoiceButtonAccessibility::actionText(const QString &actionName) const {
  if (actionName == "toggle") return "Umschalten";
  if (actionName == "increase") return "Lauter";
  if (actionName == "decrease") return "Leiser";
  return QString();
}

bool VoiceButtonAccessibility::doAction(const QString &actionName) {
  if (actionName == "toggle") {
    is_muted_ = !is_muted_;
    return true;
  }
  if (actionName == "increase") {
    volume_ = qMin(100, volume_ + 10);
    return true;
  }
  if (actionName == "decrease") {
    volume_ = qMax(0, volume_ - 10);
    return true;
  }
  return false;
}

} // namespace hh::client

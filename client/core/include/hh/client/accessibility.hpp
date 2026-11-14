// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QAccessible>
#include <QAccessibleInterface>
#include <QAccessibleValueInterface>
#include <QAccessibleTextInterface>
#include <QAccessibleStateInterface>
#include <QAccessibleActionInterface>
#include <QString>
#include <functional>

namespace hh::client {

// Accessibility-Interface für HasiHasi-Client
// Implementiert Qt Accessibility Interfaces für Screen Reader, etc.
class HasiHasiAccessibility : public QObject {
  Q_OBJECT

public:
  explicit HasiHasiAccessibility(QObject *parent = nullptr);
  ~HasiHasiAccessibility() override = default;

  // Accessibility-Unterstützung aktivieren/deaktivieren
  void enable_accessibility(bool enabled);
  bool is_enabled() const { return enabled_; }

  // UI-Elemente registrieren
  void register_widget(QWidget *widget, const QString &role, const QString &text = "");
  void unregister_widget(QWidget *widget);

  // Sprecher-Status aktualisieren
  void update_speaker_status(uint user_id, const QString &username, bool is_speaking);
  
  // Verbindungsstatus aktualisieren
  void update_connection_status(bool connected, const QString &server_address);
  
  // Kanal-Status aktualisieren
  void update_channel_status(uint channel_id, const QString &channel_name, uint user_count);

  // Barrierefreiheits-Feedback geben
  void announce(const QString &message, QAccessible::Event event = QAccessible::Announcement);

private:
  bool enabled_;
  std::map<QWidget *, QAccessible::Role> registered_widgets_;
  
  void setup_custom_roles();
};

// Custom Accessibility Interface für spezifische Widgets
class ChannelTreeAccessibility : public QAccessibleInterface,
                                public QAccessibleValueInterface,
                                public QAccessibleActionInterface {
public:
  explicit ChannelTreeAccessibility(QWidget *widget);
  ~ChannelTreeAccessibility() override = default;

  // QAccessibleInterface
  QAccessible::InterfaceType type() const override;
  QAccessible::Role role() const override;
  QAccessible::State state() const override;
  QObject *object() const override;
  QAccessibleInterface *child(QAccessible::ChildId id) const override;
  QAccessibleInterface *child(const QPoint &pos) const override;
  int childCount() const override;
  int indexOfChild(const QAccessibleInterface *child) const override;
  QString text(QAccessible::Text t, QAccessible::Relation relation = QAccessible::NoRelation,
               int relatedToIndex = -1) const override;
  QVariant value() const override;
  QVariant actionText(const QString &actionName) const override;
  bool doAction(const QString &actionName) override;

private:
  QWidget *widget_;
  QString channel_name_;
  uint user_count_;
  bool is_expanded_;
};

class VoiceButtonAccessibility : public QAccessibleInterface,
                                  public QAccessibleValueInterface,
                                  public QAccessibleActionInterface {
public:
  explicit VoiceButtonAccessibility(QWidget *widget);
  ~VoiceButtonAccessibility() override = default;

  // QAccessibleInterface
  QAccessible::InterfaceType type() const override;
  QAccessible::Role role() const override;
  QAccessible::State state() const override;
  QObject *object() const override;
  QAccessibleInterface *child(QAccessible::ChildId id) const override;
  QAccessibleInterface *child(const QPoint &pos) const override;
  int childCount() const override;
  int indexOfChild(const QAccessibleInterface *child) const override;
  QString text(QAccessible::Text t, QAccessible::Relation relation = QAccessible::NoRelation,
               int relatedToIndex = -1) const override;
  QVariant value() const override;
  QVariant actionText(const QString &actionName) const override;
  bool doAction(const QString &actionName) override;

private:
  QWidget *widget_;
  bool is_muted_;
  bool is_deafened_;
  int volume_;
};

} // namespace hh::client

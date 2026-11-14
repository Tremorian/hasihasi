// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <map>
#include <string>
#include <vector>

namespace hh::client::ui {

// Positional Audio Widget: Zeigt 3D-Positionen von Nutzern an
class PositionalAudioWidget : public QWidget {
  Q_OBJECT

public:
  explicit PositionalAudioWidget(QWidget* parent = nullptr);
  ~PositionalAudioWidget() override = default;

  // Nicht kopierbar, bewegbar
  PositionalAudioWidget(const PositionalAudioWidget&) = delete;
  PositionalAudioWidget& operator=(const PositionalAudioWidget&) = delete;
  PositionalAudioWidget(PositionalAudioWidget&&) noexcept;
  PositionalAudioWidget& operator=(PositionalAudioWidget&&) noexcept;

  // Nutzer hinzufügen/entfernen
  void add_user(uint32_t user_id, const std::string& name, float x, float y, float z);
  void remove_user(uint32_t user_id);
  void set_user_position(uint32_t user_id, float x, float y, float z);
  void set_user_speaking(uint32_t user_id, bool speaking);
  void set_user_muted(uint32_t user_id, bool muted);
  void set_user_deafened(uint32_t user_id, bool deafened);

  // Listener-Position setzen
  void set_listener_position(float x, float y, float z);

  // Zoom/Focus
  void set_zoom(float zoom);
  void focus_on_user(uint32_t user_id);

  // Statistik
  int active_users() const { return static_cast<int>(users_.size()); }
  int speaking_users() const { return speaking_users_; }
  int muted_users() const { return muted_users_; }

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

private:
  struct User {
    uint32_t id = 0;
    std::string name;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool is_speaking = false;
    bool is_muted = false;
    bool is_deafened = false;
  };

  void draw_listener(QPainter& painter);
  void draw_users(QPainter& painter);
  void draw_connections(QPainter& painter);
  void draw_labels(QPainter& painter);
  void draw_grid(QPainter& painter);

  // Nutzer-Position in Widget-Koordinaten umrechnen
  QPointF user_to_widget(float x, float y, float z) const;

  // Widget-Koordinaten in Nutzer-Position umrechnen
  void widget_to_user(const QPointF& pos, float& x, float& y, float& z) const;

  // Widget-Daten
  std::map<uint32_t, User> users_;
  float listener_x_ = 0.0f;
  float listener_y_ = 0.0f;
  float listener_z_ = 0.0f;
  float zoom_ = 1.0f;
  uint32_t focused_user_id_ = 0;

  // Statistik
  int speaking_users_ = 0;
  int muted_users_ = 0;

  // Farbpalette (Amber-CRT-Theme)
  QColor background_color_ = QColor("#0A0A08");
  QColor panel_color_ = QColor("#141410");
  QColor border_color_ = QColor("#3A2A00");
  QColor primary_color_ = QColor("#FFB000");
  QColor accent_color_ = QColor("#FFCC33");
  QColor muted_color_ = QColor("#B07800");
  QColor deafened_color_ = QColor("#FF3B1F");
  QColor grid_color_ = QColor("#3A2A00");
};

} // namespace hh::client::ui

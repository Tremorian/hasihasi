// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/ui/positionalaudiowidget.hpp"

#include "hh/logging.hpp"

#include <cmath>

namespace hh::client::ui {

PositionalAudioWidget::PositionalAudioWidget(QWidget* parent)
  : QWidget(parent) {
  setMinimumSize(400, 400);
  setMaximumSize(800, 800);
  setStyleSheet("PositionalAudioWidget { background-color: #0A0A08; border: 1px solid #3A2A00; }");

  hh::get_logger().info("PositionalAudioWidget created");
}

PositionalAudioWidget::PositionalAudioWidget(PositionalAudioWidget&&) noexcept = default;
PositionalAudioWidget& PositionalAudioWidget::operator=(PositionalAudioWidget&&) noexcept = default;

void PositionalAudioWidget::add_user(uint32_t user_id, const std::string& name, float x, float y, float z) {
  User user;
  user.id = user_id;
  user.name = name;
  user.x = x;
  user.y = y;
  user.z = z;

  users_[user_id] = user;
  update();
}

void PositionalAudioWidget::remove_user(uint32_t user_id) {
  users_.erase(user_id);
  if (focused_user_id_ == user_id) {
    focused_user_id_ = 0;
  }
  update();
}

void PositionalAudioWidget::set_user_position(uint32_t user_id, float x, float y, float z) {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    update();
  }
}

void PositionalAudioWidget::set_user_speaking(uint32_t user_id, bool speaking) {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    it->second.is_speaking = speaking;
    update();
  }
}

void PositionalAudioWidget::set_user_muted(uint32_t user_id, bool muted) {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    it->second.is_muted = muted;
    update();
  }
}

void PositionalAudioWidget::set_user_deafened(uint32_t user_id, bool deafened) {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    it->second.is_deafened = deafened;
    update();
  }
}

void PositionalAudioWidget::set_listener_position(float x, float y, float z) {
  listener_x_ = x;
  listener_y_ = y;
  listener_z_ = z;
  update();
}

void PositionalAudioWidget::set_zoom(float zoom) {
  zoom_ = zoom;
  update();
}

void PositionalAudioWidget::focus_on_user(uint32_t user_id) {
  focused_user_id_ = user_id;
  update();
}

void PositionalAudioWidget::paintEvent(QPaintEvent* event) {
  (void)event;

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Hintergrund
  painter.fillRect(rect(), background_color_);

  // Gitter zeichnen
  draw_grid(painter);

  // Verbindungen zeichnen
  draw_connections(painter);

  // Listener zeichnen
  draw_listener(painter);

  // Nutzer zeichnen
  draw_users(painter);

  // Labels zeichnen
  draw_labels(painter);

  // Statistik aktualisieren
  speaking_users_ = 0;
  muted_users_ = 0;
  for (const auto& [id, user] : users_) {
    if (user.is_speaking) speaking_users_++;
    if (user.is_muted) muted_users_++;
  }
}

void PositionalAudioWidget::draw_listener(QPainter& painter) {
  QPointF pos = user_to_widget(listener_x_, listener_y_, listener_z_);

  // Listener als Dreieck zeichnen
  painter.setPen(Qt::NoPen);
  painter.setBrush(accent_color_);

  QPolygonF triangle;
  triangle << QPointF(pos.x(), pos.y() - 10)
           << QPointF(pos.x() - 8, pos.y() + 6)
           << QPointF(pos.x() + 8, pos.y() + 6);

  painter.drawPolygon(triangle);

  // Label
  painter.setPen(primary_color_);
  painter.setFont(QFont("Monospace", 10));
  painter.drawText(pos.x() - 15, pos.y() + 20, "Listener");
}

void PositionalAudioWidget::draw_users(QPainter& painter) {
  for (const auto& [id, user] : users_) {
    QPointF pos = user_to_widget(user.x, user.y, user.z);

    // Farbe basierend auf Status
    QColor color;
    if (user.is_deafened) {
      color = deafened_color_;
    } else if (user.is_muted) {
      color = muted_color_;
    } else if (user.is_speaking) {
      color = accent_color_;
    } else {
      color = primary_color_;
    }

    // Fokus-Highlight
    if (id == focused_user_id_) {
      painter.setPen(QPen(color, 2));
      painter.setBrush(Qt::NoBrush);
      painter.drawEllipse(pos.x() - 15, pos.y() - 15, 30, 30);
    }

    // Nutzer als Kreis zeichnen
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(pos.x() - 10, pos.y() - 10, 20, 20);

    // Speaking-Indikator (Pulsieren)
    if (user.is_speaking) {
      painter.setPen(QPen(color, 1, Qt::DashLine));
      painter.setBrush(Qt::NoBrush);
      painter.drawEllipse(pos.x() - 15, pos.y() - 15, 30, 30);
    }
  }
}

void PositionalAudioWidget::draw_connections(QPainter& painter) {
  painter.setPen(QPen(grid_color_, 1, Qt::DashLine));

  for (const auto& [id, user] : users_) {
    QPointF listener_pos = user_to_widget(listener_x_, listener_y_, listener_z_);
    QPointF user_pos = user_to_widget(user.x, user.y, user.z);

    painter.drawLine(listener_pos, user_pos);
  }
}

void PositionalAudioWidget::draw_labels(QPainter& painter) {
  painter.setPen(primary_color_);
  painter.setFont(QFont("Monospace", 8));

  for (const auto& [id, user] : users_) {
    QPointF pos = user_to_widget(user.x, user.y, user.z);

    // Name
    painter.drawText(pos.x() - 20, pos.y() - 15, QString::fromStdString(user.name));

    // Position
    QString pos_text = QString("(%1, %2, %3)")
                         .arg(user.x, 0, 'f', 1)
                         .arg(user.y, 0, 'f', 1)
                         .arg(user.z, 0, 'f', 1);
    painter.drawText(pos.x() - 30, pos.y() + 25, pos_text);

    // Status-Icons
    QString status;
    if (user.is_muted) status += "🔇";
    if (user.is_deafened) status += "🔈";
    if (user.is_speaking) status += "🎤";

    if (!status.isEmpty()) {
      painter.drawText(pos.x() - 15, pos.y() + 35, status);
    }
  }
}

void PositionalAudioWidget::draw_grid(QPainter& painter) {
  painter.setPen(QPen(grid_color_, 1, Qt::DotLine));

  int width = this->width();
  int height = this->height();

  // Horizontale Linien
  for (int y = 0; y < height; y += 40) {
    painter.drawLine(0, y, width, y);
  }

  // Vertikale Linien
  for (int x = 0; x < width; x += 40) {
    painter.drawLine(x, 0, x, height);
  }
}

QPointF PositionalAudioWidget::user_to_widget(float x, float y, float z) const {
  int width = this->width();
  int height = this->height();

  // 3D zu 2D projezieren (einfache Perspektive)
  float scale = 50.0f * zoom_;
  float perspective = 1.0f / (1.0f + z * 0.1f);

  float widget_x = width / 2.0f + x * scale * perspective;
  float widget_y = height / 2.0f - y * scale * perspective;

  return QPointF(widget_x, widget_y);
}

void PositionalAudioWidget::widget_to_user(const QPointF& pos, float& x, float& y, float& z) const {
  int width = this->width();
  int height = this->height();

  // 2D zu 3D umrechnen (einfache Umkehrung)
  float scale = 50.0f * zoom_;
  float perspective = 1.0f / (1.0f + z * 0.1f);

  x = (pos.x() - width / 2.0f) / (scale * perspective);
  y = -(pos.y() - height / 2.0f) / (scale * perspective);
}

void PositionalAudioWidget::mousePressEvent(QMouseEvent* event) {
  (void)event;
  // TODO: Nutzer auswählen via Klick
  QWidget::mousePressEvent(event);
}

} // namespace hh::client::ui

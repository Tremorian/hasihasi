// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QScreen>
#include <QRect>
#include <QFont>
#include <functional>
#include <memory>

namespace hh::client {

// HiDPI-Manager für Qt6 unter Wayland/X11
// Unterstützt fraktionale Skalierung und automatische Erkennung
class HiDPIManager {
public:
  HiDPIManager() = default;
  ~HiDPIManager() = default;

  // Skalierungsfaktor abrufen
  double logical_dpi() const;
  double physical_dpi() const;
  double device_pixel_ratio() const;
  
  // Skalierung berechnen
  double calculate_scale_factor() const;
  double get_optimal_font_size(int base_size) const;
  
  // DPI-Erkennung
  bool is_high_dpi() const;
  bool is_wayland() const;
  bool is_x11() const;
  
  // Skalierung setzen (manuell)
  void set_scale_factor(double factor);
  double get_scale_factor() const;
  
  // Screen-Informationen
  struct ScreenInfo {
    int width;
    int height;
    double logical_dpi;
    double physical_dpi;
    double device_pixel_ratio;
    QRect geometry;
    QString name;
    QString manufacturer;
    QString serial;
  };
  
  ScreenInfo get_primary_screen_info() const;
  QList<ScreenInfo> get_all_screen_info() const;
  
  // DPI-Änderung erkennen
  void start_dpi_monitoring(bool enabled);
  void stop_dpi_monitoring();
  
  // Callbacks
  using ScaleChangedCallback = std::function<void(double old_factor, double new_factor)>;
  void set_scale_changed_callback(ScaleChangedCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  
  double scale_factor_;
  ScaleChangedCallback scale_changed_callback_;
  
  void init();
  void setup_screen(QScreen* screen);
};

} // namespace hh::client

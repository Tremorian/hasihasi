// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/hidpi_manager.hpp"
#include "hh/logging.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QTimer>

namespace hh::client {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct HiDPIManager::Impl {
  bool wayland_detected_ = false;
  QTimer* dpi_monitor_ = nullptr;
  
  Impl() : dpi_monitor_(nullptr) {
    // Wayland-Erkennung
    QString session_type = qgetenv("XDG_SESSION_TYPE");
    if (session_type == "wayland") {
      wayland_detected_ = true;
    } else if (session_type == "x11") {
      wayland_detected_ = false;
    } else {
      // Fallback: Prüfe QT_QPA_PLATFORM
      QString qpa = qgetenv("QT_QPA_PLATFORM");
      if (qpa.contains("wayland")) {
        wayland_detected_ = true;
      }
    }
    
    hh::get_logger().info("HiDPI: Session type: {}, Wayland: {}", 
                          session_type.toStdString(), wayland_detected_);
  }
  
  ~Impl() {
    if (dpi_monitor_) {
      delete dpi_monitor_;
    }
  }
  
  bool is_wayland() const {
    return wayland_detected_;
  }
  
  bool is_x11() const {
    return !wayland_detected_;
  }
  
  double calculate_optimal_scale(double device_pixel_ratio) {
    // Qt6 standard scale
    if (device_pixel_ratio <= 1.0) {
      return 1.0;
    } else if (device_pixel_ratio <= 1.25) {
      return 1.0;
    } else if (device_pixel_ratio <= 1.5) {
      return 1.25;
    } else if (device_pixel_ratio <= 1.75) {
      return 1.5;
    } else if (device_pixel_ratio <= 2.0) {
      return 1.75;
    } else {
      return 2.0;
    }
  }
  
  ScreenInfo get_screen_info(QScreen* screen) {
    ScreenInfo info;
    if (!screen) {
      return info;
    }
    
    info.width = screen->geometry().width();
    info.height = screen->geometry().height();
    info.logical_dpi = screen->logicalDotsPerInch();
    info.physical_dpi = screen->physicalDotsPerInch();
    info.device_pixel_ratio = screen->devicePixelRatio();
    info.geometry = screen->geometry();
    info.name = screen->name();
    info.manufacturer = screen->manufacturer();
    info.serial = screen->serial();
    
    return info;
  }
  
  void start_monitoring(HiDPIManager* manager) {
    if (dpi_monitor_) {
      return;
    }
    
    dpi_monitor_ = new QTimer();
    dpi_monitor_->setInterval(5000); // Alle 5 Sekunden prüfen
    
    QObject::connect(dpi_monitor_, &QTimer::timeout, [manager]() {
      double current_ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
      double current_scale = manager->calculate_scale_factor();
      
      // Prüfe, ob sich etwas geändert hat
      if (current_ratio != manager->get_scale_factor()) {
        manager->set_scale_factor(current_scale);
      }
    });
    
    dpi_monitor_->start();
    hh::get_logger().info("HiDPI: DPI monitoring started (5s interval)");
  }
  
  void stop_monitoring() {
    if (dpi_monitor_) {
      dpi_monitor_->stop();
      hh::get_logger().info("HiDPI: DPI monitoring stopped");
    }
  }
};

// ── HiDPIManager ──────────────────────────────────────────────────────

void HiDPIManager::init() {
  if (!impl_) {
    impl_ = std::make_unique<Impl>();
  }
  
  // Primären Screen konfigurieren
  setup_screen(QGuiApplication::primaryScreen());
  
  // Scale Factor berechnen
  scale_factor_ = calculate_scale_factor();
  
  hh::get_logger().info("HiDPI: Initialized (scale: {}, dpi: {})",
                        scale_factor_, logical_dpi());
}

double HiDPIManager::logical_dpi() const {
  if (!impl_) {
    init();
  }
  return QGuiApplication::primaryScreen()->logicalDotsPerInch();
}

double HiDPIManager::physical_dpi() const {
  if (!impl_) {
    init();
  }
  return QGuiApplication::primaryScreen()->physicalDotsPerInch();
}

double HiDPIManager::device_pixel_ratio() const {
  if (!impl_) {
    init();
  }
  return QGuiApplication::primaryScreen()->devicePixelRatio();
}

double HiDPIManager::calculate_scale_factor() const {
  if (!impl_) {
    init();
  }
  return impl_->calculate_optimal_scale(device_pixel_ratio());
}

double HiDPIManager::get_optimal_font_size(int base_size) const {
  return static_cast<int>(base_size * scale_factor_);
}

bool HiDPIManager::is_high_dpi() const {
  return device_pixel_ratio() > 1.0;
}

bool HiDPIManager::is_wayland() const {
  if (!impl_) {
    init();
  }
  return impl_->is_wayland();
}

bool HiDPIManager::is_x11() const {
  if (!impl_) {
    init();
  }
  return impl_->is_x11();
}

void HiDPIManager::set_scale_factor(double factor) {
  double old_factor = scale_factor_;
  scale_factor_ = factor;
  
  if (scale_changed_callback_) {
    scale_changed_callback_(old_factor, factor);
  }
  
  hh::get_logger().info("HiDPI: Scale factor changed from {} to {}", old_factor, factor);
}

double HiDPIManager::get_scale_factor() const {
  return scale_factor_;
}

HiDPIManager::ScreenInfo HiDPIManager::get_primary_screen_info() const {
  if (!impl_) {
    init();
  }
  return impl_->get_screen_info(QGuiApplication::primaryScreen());
}

QList<HiDPIManager::ScreenInfo> HiDPIManager::get_all_screen_info() const {
  QList<ScreenInfo> infos;
  
  if (!impl_) {
    init();
  }
  
  for (QScreen* screen : QGuiApplication::screens()) {
    infos.append(impl_->get_screen_info(screen));
  }
  
  return infos;
}

void HiDPIManager::start_dpi_monitoring(bool enabled) {
  if (!impl_) {
    init();
  }
  
  if (enabled) {
    impl_->start_monitoring(this);
  } else {
    impl_->stop_monitoring();
  }
}

void HiDPIManager::stop_dpi_monitoring() {
  if (!impl_) {
    init();
  }
  impl_->stop_monitoring();
}

void HiDPIManager::set_scale_changed_callback(ScaleChangedCallback callback) {
  scale_changed_callback_ = std::move(callback);
}

void HiDPIManager::setup_screen(QScreen* screen) {
  if (!screen) {
    return;
  }
  
  // Qt6: Fraktionale Skalierung aktivieren
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
    Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
  );
  
  hh::get_logger().info("HiDPI: Screen configured (DPR: {}, DPI: {})",
                        screen->devicePixelRatio(), screen->logicalDotsPerInch());
}

} // namespace hh::client

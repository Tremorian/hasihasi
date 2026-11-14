// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "battery_optimizer.hpp"

#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QSettings>
#include <QTimer>

namespace hh::client {

BatteryOptimizer::BatteryOptimizer(QObject* parent)
  : QObject(parent)
  , last_adaptive_check_(std::chrono::steady_clock::now()) {
  
  // Batterie-Status beobachten (wenn verfügbar)
  QTimer* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this]() {
    on_adaptive_frame_rate();
  });
  timer->start(60000); // Jede Minute prüfen
}

BatteryOptimizer::BatteryOptimizer(BatteryOptimizer&& other) noexcept
  : QObject(other.parent())
  , powerLevel_(other.powerLevel_)
  , batteryPercentage_(other.batteryPercentage_)
  , isCharging_(other.isCharging_)
  , frameRate_(other.frameRate_)
  , adaptive_frame_rate_enabled_(other.adaptive_frame_rate_enabled_)
  , backgroundAudioMode_(other.backgroundAudioMode_)
  , background_mode_enabled_(other.background_mode_enabled_)
  , max_cpu_usage_(other.max_cpu_usage_)
  , max_memory_usage_mb_(other.max_memory_usage_mb_)
  , powerSaveMode_(other.powerSaveMode_)
  , stats_(other.stats_)
  , battery_callback_(std::move(other.battery_callback_))
  , last_adaptive_check_(other.last_adaptive_check_) {
  
  other.powerLevel_ = PowerLevel::FULL;
  other.batteryPercentage_ = 100;
  other.isCharging_ = false;
  other.frameRate_ = FrameRate::MEDIUM;
  other.adaptive_frame_rate_enabled_ = true;
  other.backgroundAudioMode_ = BackgroundAudioMode::NORMAL;
  other.background_mode_enabled_ = false;
  other.max_cpu_usage_ = 0.5;
  other.max_memory_usage_mb_ = 512;
  other.powerSaveMode_ = PowerSaveMode::OFF;
  other.stats_ = Stats{};
}

BatteryOptimizer& BatteryOptimizer::operator=(BatteryOptimizer&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    powerLevel_ = other.powerLevel_;
    batteryPercentage_ = other.batteryPercentage_;
    isCharging_ = other.isCharging_;
    frameRate_ = other.frameRate_;
    adaptive_frame_rate_enabled_ = other.adaptive_frame_rate_enabled_;
    backgroundAudioMode_ = other.backgroundAudioMode_;
    background_mode_enabled_ = other.background_mode_enabled_;
    max_cpu_usage_ = other.max_cpu_usage_;
    max_memory_usage_mb_ = other.max_memory_usage_mb_;
    powerSaveMode_ = other.powerSaveMode_;
    stats_ = other.stats_;
    battery_callback_ = std::move(other.battery_callback_);
    last_adaptive_check_ = other.last_adaptive_check_;
    other.powerLevel_ = PowerLevel::FULL;
    other.batteryPercentage_ = 100;
    other.isCharging_ = false;
    other.frameRate_ = FrameRate::MEDIUM;
    other.adaptive_frame_rate_enabled_ = true;
    other.backgroundAudioMode_ = BackgroundAudioMode::NORMAL;
    other.background_mode_enabled_ = false;
    other.max_cpu_usage_ = 0.5;
    other.max_memory_usage_mb_ = 512;
    other.powerSaveMode_ = PowerSaveMode::OFF;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Energieverbrauch ─────────────────────────────────────────────────

BatteryOptimizer::PowerLevel BatteryOptimizer::power_level() const {
  return powerLevel_;
}

int BatteryOptimizer::battery_percentage() const {
  return batteryPercentage_;
}

bool BatteryOptimizer::is_charging() const {
  return isCharging_;
}

// ── Frame-Rate ───────────────────────────────────────────────────────

BatteryOptimizer::FrameRate BatteryOptimizer::frame_rate() const {
  return frameRate_;
}

void BatteryOptimizer::set_frame_rate(FrameRate rate) {
  if (frameRate_ != rate) {
    frameRate_ = rate;
    stats_.frame_rate_changes++;
    emit frame_rate_changed(rate);
    
    if (battery_callback_) {
      battery_callback_("frame_rate_changed", QVariant::fromValue(rate));
    }
  }
}

// ── Adaptive Frame-Rate ──────────────────────────────────────────────

void BatteryOptimizer::enable_adaptive_frame_rate(bool enabled) {
  adaptive_frame_rate_enabled_ = enabled;
}

bool BatteryOptimizer::is_adaptive_frame_rate_enabled() const {
  return adaptive_frame_rate_enabled_;
}

// ── Hintergrund-Audio ────────────────────────────────────────────────

BatteryOptimizer::BackgroundAudioMode BatteryOptimizer::background_audio_mode() const {
  return backgroundAudioMode_;
}

void BatteryOptimizer::set_background_audio_mode(BackgroundAudioMode mode) {
  if (backgroundAudioMode_ != mode) {
    backgroundAudioMode_ = mode;
    stats_.background_mode_changes++;
    emit background_mode_changed(mode);
    
    if (battery_callback_) {
      battery_callback_("background_mode_changed", QVariant::fromValue(mode));
    }
  }
}

void BatteryOptimizer::enable_background_mode(bool enabled) {
  background_mode_enabled_ = enabled;
}

bool BatteryOptimizer::is_background_mode_enabled() const {
  return background_mode_enabled_;
}

// ── CPU-Usage ────────────────────────────────────────────────────────

void BatteryOptimizer::set_max_cpu_usage(double max_usage) {
  max_cpu_usage_ = qBound(0.0, max_usage, 1.0);
}

double BatteryOptimizer::max_cpu_usage() const {
  return max_cpu_usage_;
}

// ── Speicher-Usage ───────────────────────────────────────────────────

void BatteryOptimizer::set_max_memory_usage_mb(int max_mb) {
  max_memory_usage_mb_ = max_mb;
}

int BatteryOptimizer::max_memory_usage_mb() const {
  return max_memory_usage_mb_;
}

// ── Statistik ────────────────────────────────────────────────────────

BatteryOptimizer::Stats BatteryOptimizer::get_stats() const {
  return stats_;
}

void BatteryOptimizer::reset_stats() {
  stats_ = Stats{};
}

// ── Energiespar-Modus ────────────────────────────────────────────────

BatteryOptimizer::PowerSaveMode BatteryOptimizer::power_save_mode() const {
  return powerSaveMode_;
}

void BatteryOptimizer::set_power_save_mode(PowerSaveMode mode) {
  if (powerSaveMode_ != mode) {
    powerSaveMode_ = mode;
    emit power_save_mode_changed(mode);
    
    if (battery_callback_) {
      battery_callback_("power_save_mode_changed", QVariant::fromValue(mode));
    }
  }
}

// ── Callbacks ────────────────────────────────────────────────────────

void BatteryOptimizer::set_battery_callback(BatteryCallback callback) {
  battery_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void BatteryOptimizer::on_battery_level_changed(int percentage) {
  batteryPercentage_ = percentage;
  
  // Power-Level bestimmen
  if (percentage < 20) {
    powerLevel_ = PowerLevel::LOW;
  } else if (percentage < 50) {
    powerLevel_ = PowerLevel::MEDIUM;
  } else if (percentage < 80) {
    powerLevel_ = PowerLevel::HIGH;
  } else {
    powerLevel_ = PowerLevel::FULL;
  }
  
  emit power_level_changed(powerLevel_, percentage);
  
  // Bei niedrigem Akku Frame-Rate reduzieren
  if (percentage < 20 && adaptive_frame_rate_enabled_) {
    set_frame_rate(FrameRate::LOW);
  }
  
  if (battery_callback_) {
    battery_callback_("battery_level_changed", QVariant::fromValue(percentage));
  }
}

void BatteryOptimizer::on_charging_changed(bool charging) {
  isCharging_ = charging;
  emit charging_changed(charging);
  
  if (charging && battery_callback_) {
    battery_callback_("charging_changed", QVariant::fromValue(charging));
  }
}

void BatteryOptimizer::on_adaptive_frame_rate() {
  if (!adaptive_frame_rate_enabled_) {
    return;
  }
  
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - last_adaptive_check_);
  
  if (elapsed.count() < 1) {
    return; // Nur jede Minute prüfen
  }
  
  last_adaptive_check_ = now;
  
  // Adaptive Frame-Rate basierend auf Akku-Status
  if (powerLevel_ == PowerLevel::LOW) {
    set_frame_rate(FrameRate::LOW);
    stats_.cpu_throttles++;
  } else if (powerLevel_ == PowerLevel::MEDIUM) {
    set_frame_rate(FrameRate::MEDIUM);
  } else {
    set_frame_rate(FrameRate::HIGH);
  }
  
  // Hintergrund-Audio anpassen
  if (powerLevel_ == PowerLevel::LOW) {
    set_background_audio_mode(BackgroundAudioMode::MINIMAL);
  } else if (powerLevel_ == PowerLevel::FULL) {
    set_background_audio_mode(BackgroundAudioMode::NORMAL);
  }
}

} // namespace hh::client

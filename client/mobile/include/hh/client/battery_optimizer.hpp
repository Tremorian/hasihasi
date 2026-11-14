// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariantMap>
#include <chrono>

namespace hh::client {

// Batterie-Optimierung: Adaptive Frame-Rate, Hintergrund-Audio nur bei Bedarf
class BatteryOptimizer : public QObject {
  Q_OBJECT

public:
  explicit BatteryOptimizer(QObject* parent = nullptr);
  ~BatteryOptimizer() override = default;

  // Nicht kopierbar, bewegbar
  BatteryOptimizer(const BatteryOptimizer&) = delete;
  BatteryOptimizer& operator=(const BatteryOptimizer&) = delete;
  BatteryOptimizer(BatteryOptimizer&&) noexcept;
  BatteryOptimizer& operator=(BatteryOptimizer&&) noexcept;

  // Energieverbrauch
  enum class PowerLevel {
    LOW,      // < 20%
    MEDIUM,   // 20-50%
    HIGH,     // 50-80%
    FULL      // > 80%
  };
  Q_ENUM(PowerLevel)

  PowerLevel power_level() const;
  int battery_percentage() const;
  bool is_charging() const;

  // Frame-Rate
  enum class FrameRate {
    LOW = 15,    // Batterie sparen
    MEDIUM = 30, // Normal
    HIGH = 60    // maximale Performance
  };
  Q_ENUM(FrameRate)

  FrameRate frame_rate() const;
  void set_frame_rate(FrameRate rate);

  // Adaptive Frame-Rate
  void enable_adaptive_frame_rate(bool enabled);
  bool is_adaptive_frame_rate_enabled() const;

  // Hintergrund-Audio
  enum class BackgroundAudioMode {
    OFF,           // Kein Hintergrund-Audio
    MINIMAL,       // Nur wichtige Events
    NORMAL         // Vollständiges Audio
  };
  Q_ENUM(BackgroundAudioMode)

  BackgroundAudioMode background_audio_mode() const;
  void set_background_audio_mode(BackgroundAudioMode mode);

  // Hintergrund-Modus
  void enable_background_mode(bool enabled);
  bool is_background_mode_enabled() const;

  // CPU-Usage
  void set_max_cpu_usage(double max_usage); // 0.0-1.0
  double max_cpu_usage() const;

  // Speicher-Usage
  void set_max_memory_usage_mb(int max_mb);
  int max_memory_usage_mb() const;

  // Statistik
  struct Stats {
    int frame_rate_changes;
    int background_mode_changes;
    int cpu_throttles;
    int memory_cleans;
    qint64 battery_saved_mah; // Milliampere-Stunden gespart
  };

  Stats get_stats() const;
  void reset_stats();

  // Energiespar-Modus
  enum class PowerSaveMode {
    OFF,
    BALANCED,
    MAXIMUM
  };
  Q_ENUM(PowerSaveMode)

  PowerSaveMode power_save_mode() const;
  void set_power_save_mode(PowerSaveMode mode);

  // Callbacks
  using BatteryCallback = std::function<void(const QString& event, const QVariant& data)>;
  void set_battery_callback(BatteryCallback callback);

signals:
  void power_level_changed(PowerLevel level, int percentage);
  void frame_rate_changed(FrameRate rate);
  void background_mode_changed(BackgroundAudioMode mode);
  void power_save_mode_changed(PowerSaveMode mode);
  void battery_low(int percentage);
  void charging_changed(bool charging);
  void error_occurred(const QString& error);

private slots:
  void on_battery_level_changed(int percentage);
  void on_charging_changed(bool charging);
  void on_adaptive_frame_rate();

private:
  PowerLevel powerLevel_ = PowerLevel::FULL;
  int batteryPercentage_ = 100;
  bool isCharging_ = false;
  
  FrameRate frameRate_ = FrameRate::MEDIUM;
  bool adaptive_frame_rate_enabled_ = true;
  
  BackgroundAudioMode backgroundAudioMode_ = BackgroundAudioMode::NORMAL;
  bool background_mode_enabled_ = false;
  
  double max_cpu_usage_ = 0.5; // 50%
  int max_memory_usage_mb_ = 512;
  
  PowerSaveMode powerSaveMode_ = PowerSaveMode::OFF;
  
  Stats stats_;
  
  BatteryCallback battery_callback_;
  
  std::chrono::steady_clock::time_point last_adaptive_check_;
};

} // namespace hh::client

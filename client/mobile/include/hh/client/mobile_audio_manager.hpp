// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioSource>
#include <QMediaDevices>
#include <functional>

namespace hh::client {

// Mobile-Audio-Management für Headset, Background-Mode, Do-not-disturb
class MobileAudioManager : public QObject {
  Q_OBJECT

public:
  explicit MobileAudioManager(QObject* parent = nullptr);
  ~MobileAudioManager() override = default;

  // Nicht kopierbar, bewegbar
  MobileAudioManager(const MobileAudioManager&) = delete;
  MobileAudioManager& operator=(const MobileAudioManager&) = delete;
  MobileAudioManager(MobileAudioManager&&) noexcept;
  MobileAudioManager& operator=(MobileAudioManager&&) noexcept;

  // Audio-Geräte-Verwaltung
  struct AudioDevice {
    QString id;
    QString name;
    bool isDefault;
    bool isAvailable;
  };

  std::vector<AudioDevice> get_input_devices() const;
  std::vector<AudioDevice> get_output_devices() const;

  // Headset-Management
  enum class HeadsetType {
    NONE,
    BLUETOOTH,
    WIRED,
    USB
  };
  Q_ENUM(HeadsetType)

  HeadsetType headsetType() const;
  bool is_headset_connected() const;
  QString get_connected_headset_name() const;

  // Headset-Ereignisse
  void set_headset_callback(std::function<void(HeadsetType, const QString&)> callback);

  // Background-Mode
  enum class AudioSessionMode {
    ACTIVE,
    BACKGROUND,
    MUTED
  };
  Q_ENUM(AudioSessionMode)

  AudioSessionMode sessionMode() const;
  void set_session_mode(AudioSessionMode mode);

  // Background-Mode konfigurieren
  void enable_background_mode(bool enabled);
  bool is_background_mode_enabled() const;

  // Do-not-disturb
  enum class DNDMode {
    OFF,
    CALLS_ONLY,
    ALARM_ONLY,
    FULL
  };
  Q_ENUM(DNDMode)

  DNDMode dndMode() const;
  void set_dnd_mode(DNDMode mode);

  // DND-Ereignisse
  void set_dnd_callback(std::function<void(DNDMode)> callback);

  // Audio-Priorität
  void set_audio_priority(int priority); // 0-100
  int audio_priority() const;

  // Audio-Route
  enum class AudioRoute {
    SPEAKER,
    EARPIECE,
    BLUETOOTH,
    WIRED,
    USB
  };
  Q_ENUM(AudioRoute)

  AudioRoute audio_route() const;
  bool set_audio_route(AudioRoute route);

  // Statistik
  struct Stats {
    int active_connections;
    int headset_changes;
    int mode_changes;
    int route_changes;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using AudioCallback = std::function<void(const QString& event, const QVariant& data)>;
  void set_audio_callback(AudioCallback callback);

signals:
  void headset_changed(HeadsetType type, const QString& name);
  void session_mode_changed(AudioSessionMode mode);
  void dnd_mode_changed(DNDMode mode);
  void audio_route_changed(AudioRoute route);
  void error_occurred(const QString& error);

private slots:
  void on_audio_device_changed(const QList<QAudioDevice>& devices);
  void on_session_mode_changed(AudioSessionMode mode);
  void on_dnd_mode_changed(DNDMode mode);

private:
  HeadsetType headsetType_ = HeadsetType::NONE;
  QString connected_headset_name_;
  AudioSessionMode sessionMode_ = AudioSessionMode::ACTIVE;
  DNDMode dndMode_ = DNDMode::OFF;
  int audio_priority_ = 50;
  AudioRoute audioRoute_ = AudioRoute::SPEAKER;
  bool background_mode_enabled_ = false;
  
  std::function<void(HeadsetType, const QString&)> headset_callback_;
  std::function<void(DNDMode)> dnd_callback_;
  AudioCallback audio_callback_;
  
  Stats stats_;
};

} // namespace hh::client

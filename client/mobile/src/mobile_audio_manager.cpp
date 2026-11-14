// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mobile_audio_manager.hpp"

#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QSettings>

namespace hh::client {

MobileAudioManager::MobileAudioManager(QObject* parent)
  : QObject(parent) {
  
  // Audio-Geräte-Änderungen beobachten
  connect(
    QMediaDevices::defaultInstance(),
    &QMediaDevices::audioInputsChanged,
    this,
    &MobileAudioManager::on_audio_device_changed
  );
  
  connect(
    QMediaDevices::defaultInstance(),
    &QMediaDevices::audioOutputsChanged,
    this,
    &MobileAudioManager::on_audio_device_changed
  );
}

MobileAudioManager::MobileAudioManager(MobileAudioManager&& other) noexcept
  : QObject(other.parent())
  , headsetType_(other.headsetType_)
  , connected_headset_name_(std::move(other.connected_headset_name_))
  , sessionMode_(other.sessionMode_)
  , dndMode_(other.dndMode_)
  , audio_priority_(other.audio_priority_)
  , audioRoute_(other.audioRoute_)
  , background_mode_enabled_(other.background_mode_enabled_)
  , headset_callback_(std::move(other.headset_callback_))
  , dnd_callback_(std::move(other.dnd_callback_))
  , audio_callback_(std::move(other.audio_callback_))
  , stats_(other.stats_) {
  
  other.headsetType_ = HeadsetType::NONE;
  other.sessionMode_ = AudioSessionMode::ACTIVE;
  other.dndMode_ = DNDMode::OFF;
  other.audio_priority_ = 50;
  other.audioRoute_ = AudioRoute::SPEAKER;
  other.background_mode_enabled_ = false;
  other.stats_ = Stats{};
}

MobileAudioManager& MobileAudioManager::operator=(MobileAudioManager&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    headsetType_ = other.headsetType_;
    connected_headset_name_ = std::move(other.connected_headset_name_);
    sessionMode_ = other.sessionMode_;
    dndMode_ = other.dndMode_;
    audio_priority_ = other.audio_priority_;
    audioRoute_ = other.audioRoute_;
    background_mode_enabled_ = other.background_mode_enabled_;
    headset_callback_ = std::move(other.headset_callback_);
    dnd_callback_ = std::move(other.dnd_callback_);
    audio_callback_ = std::move(other.audio_callback_);
    stats_ = other.stats_;
    other.headsetType_ = HeadsetType::NONE;
    other.sessionMode_ = AudioSessionMode::ACTIVE;
    other.dndMode_ = DNDMode::OFF;
    other.audio_priority_ = 50;
    other.audioRoute_ = AudioRoute::SPEAKER;
    other.background_mode_enabled_ = false;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Audio-Geräte-Verwaltung ──────────────────────────────────────────

std::vector<MobileAudioManager::AudioDevice> MobileAudioManager::get_input_devices() const {
  std::vector<AudioDevice> devices;
  
  QMediaDevices* mediaDevices = QMediaDevices::defaultInstance();
  QList<QAudioDevice> inputDevices = mediaDevices->audioInputs();
  
  for (const QAudioDevice& device : inputDevices) {
    AudioDevice audioDevice;
    audioDevice.id = device.id();
    audioDevice.name = device.description();
    audioDevice.isDefault = device.isDefault();
    audioDevice.isAvailable = device.isNull() ? false : true;
    devices.push_back(audioDevice);
  }
  
  return devices;
}

std::vector<MobileAudioManager::AudioDevice> MobileAudioManager::get_output_devices() const {
  std::vector<AudioDevice> devices;
  
  QMediaDevices* mediaDevices = QMediaDevices::defaultInstance();
  QList<QAudioDevice> outputDevices = mediaDevices->audioOutputs();
  
  for (const QAudioDevice& device : outputDevices) {
    AudioDevice audioDevice;
    audioDevice.id = device.id();
    audioDevice.name = device.description();
    audioDevice.isDefault = device.isDefault();
    audioDevice.isAvailable = device.isNull() ? false : true;
    devices.push_back(audioDevice);
  }
  
  return devices;
}

// ── Headset-Management ───────────────────────────────────────────────

MobileAudioManager::HeadsetType MobileAudioManager::headsetType() const {
  return headsetType_;
}

bool MobileAudioManager::is_headset_connected() const {
  return headsetType_ != HeadsetType::NONE;
}

QString MobileAudioManager::get_connected_headset_name() const {
  return connected_headset_name_;
}

void MobileAudioManager::set_headset_callback(std::function<void(HeadsetType, const QString&)> callback) {
  headset_callback_ = std::move(callback);
}

// ── Background-Mode ──────────────────────────────────────────────────

MobileAudioManager::AudioSessionMode MobileAudioManager::sessionMode() const {
  return sessionMode_;
}

void MobileAudioManager::set_session_mode(AudioSessionMode mode) {
  if (sessionMode_ != mode) {
    sessionMode_ = mode;
    stats_.mode_changes++;
    emit session_mode_changed(mode);
    
    if (audio_callback_) {
      audio_callback_("session_mode_changed", QVariant::fromValue(mode));
    }
  }
}

void MobileAudioManager::enable_background_mode(bool enabled) {
  background_mode_enabled_ = enabled;
}

bool MobileAudioManager::is_background_mode_enabled() const {
  return background_mode_enabled_;
}

// ── Do-not-disturb ───────────────────────────────────────────────────

MobileAudioManager::DNDMode MobileAudioManager::dndMode() const {
  return dndMode_;
}

void MobileAudioManager::set_dnd_mode(DNDMode mode) {
  if (dndMode_ != mode) {
    dndMode_ = mode;
    stats_.mode_changes++;
    emit dnd_mode_changed(mode);
    
    if (dnd_callback_) {
      dnd_callback_(mode);
    }
    
    if (audio_callback_) {
      audio_callback_("dnd_mode_changed", QVariant::fromValue(mode));
    }
  }
}

void MobileAudioManager::set_dnd_callback(std::function<void(DNDMode)> callback) {
  dnd_callback_ = std::move(callback);
}

// ── Audio-Priorität ──────────────────────────────────────────────────

void MobileAudioManager::set_audio_priority(int priority) {
  audio_priority_ = qBound(0, priority, 100);
}

int MobileAudioManager::audio_priority() const {
  return audio_priority_;
}

// ── Audio-Route ──────────────────────────────────────────────────────

MobileAudioManager::AudioRoute MobileAudioManager::audio_route() const {
  return audioRoute_;
}

bool MobileAudioManager::set_audio_route(AudioRoute route) {
  if (audioRoute_ != route) {
    audioRoute_ = route;
    stats_.route_changes++;
    emit audio_route_changed(route);
    
    if (audio_callback_) {
      audio_callback_("audio_route_changed", QVariant::fromValue(route));
    }
    return true;
  }
  return false;
}

// ── Statistik ────────────────────────────────────────────────────────

MobileAudioManager::Stats MobileAudioManager::get_stats() const {
  return stats_;
}

void MobileAudioManager::reset_stats() {
  stats_ = Stats{};
}

// ── Callbacks ────────────────────────────────────────────────────────

void MobileAudioManager::set_audio_callback(AudioCallback callback) {
  audio_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void MobileAudioManager::on_audio_device_changed(const QList<QAudioDevice>& devices) {
  // Headset-Erkennung basierend auf Geräte-IDs
  for (const QAudioDevice& device : devices) {
    QString id = device.id();
    
    // Bluetooth-Headset erkennen
    if (id.contains("bluetooth", Qt::CaseInsensitive)) {
      if (headsetType_ != HeadsetType::BLUETOOTH) {
        headsetType_ = HeadsetType::BLUETOOTH;
        connected_headset_name_ = device.description();
        stats_.headset_changes++;
        emit headset_changed(HeadsetType::BLUETOOTH, device.description());
        
        if (headset_callback_) {
          headset_callback_(HeadsetType::BLUETOOTH, device.description());
        }
      }
    }
    // Kabelgebundenes Headset erkennen
    else if (id.contains("wired", Qt::CaseInsensitive) || id.contains("headset", Qt::CaseInsensitive)) {
      if (headsetType_ != HeadsetType::WIRED) {
        headsetType_ = HeadsetType::WIRED;
        connected_headset_name_ = device.description();
        stats_.headset_changes++;
        emit headset_changed(HeadsetType::WIRED, device.description());
        
        if (headset_callback_) {
          headset_callback_(HeadsetType::WIRED, device.description());
        }
      }
    }
    // USB-Headset erkennen
    else if (id.contains("usb", Qt::CaseInsensitive)) {
      if (headsetType_ != HeadsetType::USB) {
        headsetType_ = HeadsetType::USB;
        connected_headset_name_ = device.description();
        stats_.headset_changes++;
        emit headset_changed(HeadsetType::USB, device.description());
        
        if (headset_callback_) {
          headset_callback_(HeadsetType::USB, device.description());
        }
      }
    }
  }
}

void MobileAudioManager::on_session_mode_changed(AudioSessionMode mode) {
  set_session_mode(mode);
}

void MobileAudioManager::on_dnd_mode_changed(DNDMode mode) {
  set_dnd_mode(mode);
}

} // namespace hh::client

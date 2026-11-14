// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screen_share_manager.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QOperatingSystemVersion>

namespace hh::client {

ScreenShareManager::ScreenShareManager(QObject* parent)
  : QObject(parent) {
  
  // Display-Server erkennen
  QOperatingSystemVersion os = QOperatingSystemVersion::current();
  if (os.type() == QOperatingSystemVersion::Linux) {
    // Prüfe, ob X11 oder Wayland läuft
    QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
    if (sessionType == "wayland") {
      display_server_ = DisplayServer::WAYLAND;
    } else if (sessionType == "x11" || qEnvironmentVariableIsSet("DISPLAY")) {
      display_server_ = DisplayServer::X11;
    }
  }
  
  // Bildschirme erkennen
  auto screens = QGuiApplication::screens();
  for (int i = 0; i < screens.size(); i++) {
    ScreenInfo info;
    info.id = i;
    info.name = screens[i]->name();
    info.width = screens[i]->geometry().width();
    info.height = screens[i]->geometry().height();
    info.dpi = static_cast<int>(screens[i]->logicalDotsPerInch());
    info.geometry = screens[i]->geometry();
    screens_.push_back(info);
  }
  
  // Fenster-Erkennung (simuliert)
  // In einer echten Implementierung würde hier X11/XCB oder Wayland verwendet
  WindowInfo info;
  info.name = "HasiHasi";
  info.id = 123456;
  info.x = 0;
  info.y = 0;
  info.width = 1280;
  info.height = 720;
  windows_.push_back(info);
}

ScreenShareManager::ScreenShareManager(ScreenShareManager&& other) noexcept
  : QObject(other.parent())
  , display_server_(other.display_server_)
  , video_codec_(other.video_codec_)
  , quality_(other.quality_)
  , framerate_(other.framerate_)
  , status_(other.status_)
  , capture_region_(other.capture_region_)
  , full_screen_capture_(other.full_screen_capture_)
  , screens_(std::move(other.screens_))
  , active_screen_id_(other.active_screen_id_)
  , last_frame_(other.last_frame_)
  , bitrate_kbps_(other.bitrate_kbps_)
  , stats_(other.stats_)
  , frame_callback_(std::move(other.frame_callback_))
  , error_callback_(std::move(other.error_callback_)) {
  
  other.display_server_ = DisplayServer::NONE;
  other.video_codec_ = VideoCodec::NONE;
  other.quality_ = Quality::LOW;
  other.framerate_ = Framerate::FPS_15;
  other.status_ = ShareStatus::IDLE;
  other.capture_region_ = {0, 0, 0, 0};
  other.full_screen_capture_ = true;
  other.active_screen_id_ = 0;
  other.bitrate_kbps_ = 0;
  other.stats_ = Stats{};
}

ScreenShareManager& ScreenShareManager::operator=(ScreenShareManager&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    display_server_ = other.display_server_;
    video_codec_ = other.video_codec_;
    quality_ = other.quality_;
    framerate_ = other.framerate_;
    status_ = other.status_;
    capture_region_ = other.capture_region_;
    full_screen_capture_ = other.full_screen_capture_;
    screens_ = std::move(other.screens_);
    active_screen_id_ = other.active_screen_id_;
    last_frame_ = other.last_frame_;
    bitrate_kbps_ = other.bitrate_kbps_;
    stats_ = other.stats_;
    frame_callback_ = std::move(other.frame_callback_);
    error_callback_ = std::move(other.error_callback_);
    other.display_server_ = DisplayServer::NONE;
    other.video_codec_ = VideoCodec::NONE;
    other.quality_ = Quality::LOW;
    other.framerate_ = Framerate::FPS_15;
    other.status_ = ShareStatus::IDLE;
    other.capture_region_ = {0, 0, 0, 0};
    other.full_screen_capture_ = true;
    other.active_screen_id_ = 0;
    other.bitrate_kbps_ = 0;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Display-Server ───────────────────────────────────────────────────

ScreenShareManager::DisplayServer ScreenShareManager::display_server() const {
  return display_server_;
}

// ── Bildschirm-Erkennung ─────────────────────────────────────────────

std::vector<ScreenShareManager::ScreenInfo> ScreenShareManager::get_available_screens() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return screens_;
}

ScreenShareManager::ScreenInfo* ScreenShareManager::get_primary_screen() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!screens_.empty()) {
    return &screens_[0];
  }
  return nullptr;
}

const ScreenShareManager::ScreenInfo* ScreenShareManager::get_primary_screen() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!screens_.empty()) {
    return &screens_[0];
  }
  return nullptr;
}

// ── Fenster-Erfassung ────────────────────────────────────────────────

std::vector<ScreenShareManager::WindowInfo> ScreenShareManager::get_available_windows() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return windows_;
}

ScreenShareManager::WindowInfo* ScreenShareManager::get_focused_window() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!windows_.empty()) {
    return &windows_[0];
  }
  return nullptr;
}

const ScreenShareManager::WindowInfo* ScreenShareManager::get_focused_window() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!windows_.empty()) {
    return &windows_[0];
  }
  return nullptr;
}

bool ScreenShareManager::set_target_window(unsigned long window_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  target_window_id_ = window_id;
  return true;
}

// ── Video-Codec ──────────────────────────────────────────────────────

ScreenShareManager::VideoCodec ScreenShareManager::video_codec() const {
  return video_codec_;
}

bool ScreenShareManager::set_video_codec(VideoCodec codec) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ != ShareStatus::IDLE) {
    return false; // Darf nicht während des Shares geändert werden
  }
  
  if (video_codec_ != codec) {
    video_codec_ = codec;
    emit codec_changed(codec);
    return true;
  }
  return false;
}

// ── Qualität ─────────────────────────────────────────────────────────

ScreenShareManager::Quality ScreenShareManager::quality() const {
  return quality_;
}

bool ScreenShareManager::set_quality(Quality q) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (quality_ != q) {
    quality_ = q;
    emit quality_changed(q);
    return true;
  }
  return false;
}

// ── Framerate ────────────────────────────────────────────────────────

ScreenShareManager::Framerate ScreenShareManager::framerate() const {
  return framerate_;
}

bool ScreenShareManager::set_framerate(Framerate fr) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (framerate_ != fr) {
    framerate_ = fr;
    emit framerate_changed(fr);
    return true;
  }
  return false;
}

// ── Region Capture ───────────────────────────────────────────────────

ScreenShareManager::CaptureRegion ScreenShareManager::get_capture_region() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return capture_region_;
}

void ScreenShareManager::set_capture_region(const CaptureRegion& region) {
  std::lock_guard<std::mutex> lock(mutex_);
  capture_region_ = region;
}

void ScreenShareManager::set_full_screen_capture(bool full) {
  std::lock_guard<std::mutex> lock(mutex_);
  full_screen_capture_ = full;
}

// ── Audio-Integration ────────────────────────────────────────────────

ScreenShareManager::AudioSource ScreenShareManager::audio_source() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return audio_source_;
}

bool ScreenShareManager::set_audio_source(AudioSource source) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (audio_source_ != source) {
    audio_source_ = source;
    audio_enabled_ = (source != AudioSource::NONE);
    emit audio_source_changed(source);
    return true;
  }
  return false;
}

bool ScreenShareManager::is_audio_enabled() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return audio_enabled_;
}

// ── Aufzeichnung ─────────────────────────────────────────────────────

ScreenShareManager::RecordStatus ScreenShareManager::record_status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return record_status_;
}

bool ScreenShareManager::start_recording(const QString& file_path) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (record_status_ != RecordStatus::IDLE) {
    return false;
  }
  
  recorded_file_path_ = file_path;
  record_status_ = RecordStatus::RECORDING;
  
  emit recording_started(file_path);
  return true;
}

bool ScreenShareManager::stop_recording() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (record_status_ == RecordStatus::IDLE) {
    return false;
  }
  
  record_status_ = RecordStatus::IDLE;
  emit recording_stopped();
  return true;
}

bool ScreenShareManager::pause_recording() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (record_status_ != RecordStatus::RECORDING) {
    return false;
  }
  
  record_status_ = RecordStatus::PAUSED;
  emit recording_paused();
  return true;
}

bool ScreenShareManager::resume_recording() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (record_status_ != RecordStatus::PAUSED) {
    return false;
  }
  
  record_status_ = RecordStatus::RECORDING;
  emit recording_resumed();
  return true;
}

QString ScreenShareManager::get_recorded_file_path() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return recorded_file_path_;
}

// ── Share-Status ─────────────────────────────────────────────────────

ScreenShareManager::ShareStatus ScreenShareManager::status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return status_;
}

// ── Share-Start/Stop ─────────────────────────────────────────────────

bool ScreenShareManager::start_share(int screen_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ != ShareStatus::IDLE) {
    return false;
  }
  
  if (screen_id < 0 || screen_id >= static_cast<int>(screens_.size())) {
    return false;
  }
  
  active_screen_id_ = screen_id;
  status_ = ShareStatus::CAPTURING;
  
  // In einer echten Implementierung würde hier die Capture-Logik starten
  emit share_started(screen_id);
  return true;
}

bool ScreenShareManager::stop_share() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ == ShareStatus::IDLE) {
    return false;
  }
  
  status_ = ShareStatus::IDLE;
  emit share_stopped();
  return true;
}

bool ScreenShareManager::pause_share() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ != ShareStatus::STREAMING) {
    return false;
  }
  
  status_ = ShareStatus::PAUSED;
  emit share_paused();
  return true;
}

bool ScreenShareManager::resume_share() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ != ShareStatus::PAUSED) {
    return false;
  }
  
  status_ = ShareStatus::STREAMING;
  emit share_resumed();
  return true;
}

// ── Bild-Erfassung ───────────────────────────────────────────────────

ScreenShareManager::CapturedFrame ScreenShareManager::get_last_frame() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_frame_;
}

// ── Bitrate ──────────────────────────────────────────────────────────

void ScreenShareManager::set_bitrate(int kbps) {
  std::lock_guard<std::mutex> lock(mutex_);
  bitrate_kbps_ = kbps;
}

int ScreenShareManager::bitrate() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return bitrate_kbps_;
}

// ── Qualitätseinstellungen ───────────────────────────────────────────

ScreenShareManager::QualitySettings ScreenShareManager::get_quality_settings() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  QualitySettings settings;
  
  switch (quality_) {
    case Quality::LOW:
      settings.width = 640;
      settings.height = 480;
      settings.bitrate_kbps = 500;
      settings.max_framerate = 15;
      settings.keyframe_interval = 2;
      break;
    case Quality::MEDIUM:
      settings.width = 1280;
      settings.height = 720;
      settings.bitrate_kbps = 2000;
      settings.max_framerate = 30;
      settings.keyframe_interval = 1;
      break;
    case Quality::HIGH:
      settings.width = 1920;
      settings.height = 1080;
      settings.bitrate_kbps = 5000;
      settings.max_framerate = 30;
      settings.keyframe_interval = 1;
      break;
    case Quality::ULTRA:
      settings.width = 2560;
      settings.height = 1440;
      settings.bitrate_kbps = 10000;
      settings.max_framerate = 60;
      settings.keyframe_interval = 1;
      break;
    default:
      settings.width = 1280;
      settings.height = 720;
      settings.bitrate_kbps = 2000;
      settings.max_framerate = 30;
      settings.keyframe_interval = 1;
      break;
  }
  
  return settings;
}

// ── Statistik ────────────────────────────────────────────────────────

ScreenShareManager::Stats ScreenShareManager::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void ScreenShareManager::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
  default_permission_ = PermissionLevel::NONE;
}

// ── Callbacks ────────────────────────────────────────────────────────

void ScreenShareManager::set_frame_callback(FrameCallback callback) {
  frame_callback_ = std::move(callback);
}

void ScreenShareManager::set_error_callback(ErrorCallback callback) {
  error_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void ScreenShareManager::on_capture_timer() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (status_ != ShareStatus::CAPTURING && status_ != ShareStatus::STREAMING) {
    return;
  }
  
  // Simuliere Frame-Erfassung
  CapturedFrame frame;
  frame.width = 1280;
  frame.height = 720;
  frame.timestamp = QDateTime::currentMSecsSinceEpoch() * 1000; // µs
  
  // In einer echten Implementierung würde hier der Bildschirm erfasst
  // frame.image = capture_screen();
  
  stats_.frames_captured++;
  stats_.frames_sent++;
  
  emit frame_captured(frame);
  
  if (frame_callback_) {
    frame_callback_(frame);
  }
}

// ── Berechtigung ─────────────────────────────────────────────────────

bool ScreenShareManager::has_permission(uint32_t user_id, PermissionLevel level) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = permissions_.find(user_id);
  if (it == permissions_.end()) {
    return false;
  }
  
  for (const auto& perm : it->second) {
    if (perm.is_granted && static_cast<int>(perm.level) >= static_cast<int>(level)) {
      return true;
    }
  }
  
  return false;
}

bool ScreenShareManager::grant_permission(uint32_t user_id, const QString& username, PermissionLevel level, uint32_t granted_by, const QString& reason) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  Permission perm;
  perm.user_id = user_id;
  perm.username = username;
  perm.level = level;
  perm.is_granted = true;
  perm.granted_at = QDateTime::currentDateTime();
  perm.granted_by = granted_by;
  perm.reason = reason;
  
  permissions_[user_id].push_back(perm);
  return true;
}

bool ScreenShareManager::revoke_permission(uint32_t user_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = permissions_.find(user_id);
  if (it != permissions_.end()) {
    for (auto& perm : it->second) {
      perm.is_granted = false;
    }
    return true;
  }
  
  return false;
}

std::vector<ScreenShareManager::Permission> ScreenShareManager::get_permissions(uint32_t user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = permissions_.find(user_id);
  if (it != permissions_.end()) {
    return it->second;
  }
  
  return {};
}

std::vector<ScreenShareManager::Permission> ScreenShareManager::get_all_permissions() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Permission> result;
  for (const auto& pair : permissions_) {
    for (const auto& perm : pair.second) {
      result.push_back(perm);
    }
  }
  
  return result;
}

ScreenShareManager::PermissionLevel ScreenShareManager::get_user_permission(uint32_t user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = permissions_.find(user_id);
  if (it == permissions_.end()) {
    return default_permission_;
  }
  
  PermissionLevel highest = default_permission_;
  for (const auto& perm : it->second) {
    if (perm.is_granted && static_cast<int>(perm.level) > static_cast<int>(highest)) {
      highest = perm.level;
    }
  }
  
  return highest;
}

void ScreenShareManager::set_default_permission(PermissionLevel level) {
  std::lock_guard<std::mutex> lock(mutex_);
  default_permission_ = level;
}

ScreenShareManager::PermissionLevel ScreenShareManager::default_permission() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return default_permission_;
}

} // namespace hh::client

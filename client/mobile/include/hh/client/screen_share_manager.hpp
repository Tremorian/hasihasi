// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QRect>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace hh::client {

// Screenshare: X11/Wayland, Video-Codec, Qualität, Framerate
class ScreenShareManager : public QObject {
  Q_OBJECT

public:
  explicit ScreenShareManager(QObject* parent = nullptr);
  ~ScreenShareManager() override = default;

  // Nicht kopierbar, bewegbar
  ScreenShareManager(const ScreenShareManager&) = delete;
  ScreenShareManager& operator=(const ScreenShareManager&) = delete;
  ScreenShareManager(ScreenShareManager&&) noexcept;
  ScreenShareManager& operator=(ScreenShareManager&&) noexcept;

  // Display-Server
  enum class DisplayServer {
    NONE,
    X11,
    WAYLAND
  };
  Q_ENUM(DisplayServer)

  DisplayServer display_server() const;

  // Bildschirm-Erkennung
  struct ScreenInfo {
    QString name;
    int id;
    int width;
    int height;
    int dpi;
    QRect geometry;
  };

  std::vector<ScreenInfo> get_available_screens() const;
  ScreenInfo* get_primary_screen();
  const ScreenInfo* get_primary_screen() const;

  // Fenster-Erfassung
  struct WindowInfo {
    QString name;
    unsigned long id;
    int x;
    int y;
    int width;
    int height;
  };

  std::vector<WindowInfo> get_available_windows() const;
  WindowInfo* get_focused_window();
  const WindowInfo* get_focused_window() const;
  bool set_target_window(unsigned long window_id);

  // Video-Codec
  enum class VideoCodec {
    NONE,
    VP8,
    VP9,
    H264
  };
  Q_ENUM(VideoCodec)

  VideoCodec video_codec() const;
  bool set_video_codec(VideoCodec codec);

  // Qualität
  enum class Quality {
    LOW,
    MEDIUM,
    HIGH,
    ULTRA
  };
  Q_ENUM(Quality)

  Quality quality() const;
  bool set_quality(Quality q);

  // Framerate
  enum class Framerate {
    FPS_15,
    FPS_30,
    FPS_60
  };
  Q_ENUM(Framerate)

  Framerate framerate() const;
  bool set_framerate(Framerate fr);

  // Region Capture
  struct CaptureRegion {
    int x;
    int y;
    int width;
    int height;
  };

  CaptureRegion get_capture_region() const;
  void set_capture_region(const CaptureRegion& region);
  void set_full_screen_capture(bool full);

  // Audio-Integration
  enum class AudioSource {
    NONE,
    MICROPHONE,
    SYSTEM_AUDIO,
    BOTH
  };
  Q_ENUM(AudioSource)

  AudioSource audio_source() const;
  bool set_audio_source(AudioSource source);
  bool is_audio_enabled() const;

  // Aufzeichnung
  enum class RecordStatus {
    IDLE,
    RECORDING,
    PAUSED,
    ERROR
  };
  Q_ENUM(RecordStatus)

  RecordStatus record_status() const;
  bool start_recording(const QString& file_path);
  bool stop_recording();
  bool pause_recording();
  bool resume_recording();
  QString get_recorded_file_path() const;

  // Berechtigung
  enum class PermissionLevel {
    NONE,
    VIEW_ONLY,
    SHARE_SCREEN,
    CONTROL,
    ADMIN
  };
  Q_ENUM(PermissionLevel)

  struct Permission {
    uint32_t user_id;
    QString username;
    PermissionLevel level;
    bool is_granted;
    QDateTime granted_at;
    uint32_t granted_by;
    QString reason;
  };

  bool has_permission(uint32_t user_id, PermissionLevel level) const;
  bool grant_permission(uint32_t user_id, const QString& username, PermissionLevel level, uint32_t granted_by, const QString& reason);
  bool revoke_permission(uint32_t user_id);
  std::vector<Permission> get_permissions(uint32_t user_id) const;
  std::vector<Permission> get_all_permissions() const;
  PermissionLevel get_user_permission(uint32_t user_id) const;
  void set_default_permission(PermissionLevel level);
  PermissionLevel default_permission() const;

  // Share-Status
  enum class ShareStatus {
    IDLE,
    CAPTURING,
    STREAMING,
    PAUSED,
    ERROR
  };
  Q_ENUM(ShareStatus)

  ShareStatus status() const;

  // Share-Start/Stop
  bool start_share(int screen_id = 0);
  bool stop_share();
  bool pause_share();
  bool resume_share();

  // Bild-Erfassung
  struct CapturedFrame {
    QImage image;
    qint64 timestamp; // µs
    int width;
    int height;
  };

  CapturedFrame get_last_frame() const;

  // Bitrate
  void set_bitrate(int kbps); // 0 = auto
  int bitrate() const;

  // Qualitätseinstellungen
  struct QualitySettings {
    int width;
    int height;
    int bitrate_kbps;
    int max_framerate;
    int keyframe_interval; // Sekunden
  };

  QualitySettings get_quality_settings() const;

  // Statistik
  struct Stats {
    size_t frames_captured;
    size_t frames_sent;
    size_t frames_dropped;
    qint64 total_capture_time_ms;
    double average_bitrate_kbps;
    double average_framerate;
    qint64 recording_duration_ms;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using FrameCallback = std::function<void(const CapturedFrame& frame)>;
  using ErrorCallback = std::function<void(const QString& error)>;

  void set_frame_callback(FrameCallback callback);
  void set_error_callback(ErrorCallback callback);

signals:
  void share_started(int screen_id);
  void share_stopped();
  void share_paused();
  void share_resumed();
  void frame_captured(const CapturedFrame& frame);
  void error_occurred(const QString& error);
  void quality_changed(Quality q);
  void codec_changed(VideoCodec codec);
  void framerate_changed(Framerate fr);
  void audio_source_changed(AudioSource source);
  void recording_started(const QString& file_path);
  void recording_stopped();
  void recording_paused();
  void recording_resumed();

private slots:
  void on_capture_timer();

private:
  DisplayServer display_server_ = DisplayServer::NONE;
  VideoCodec video_codec_ = VideoCodec::VP8;
  Quality quality_ = Quality::MEDIUM;
  Framerate framerate_ = Framerate::FPS_30;
  ShareStatus status_ = ShareStatus::IDLE;
  
  CaptureRegion capture_region_ = {0, 0, 0, 0};
  bool full_screen_capture_ = true;
  
  std::vector<ScreenInfo> screens_;
  int active_screen_id_ = 0;
  
  std::vector<WindowInfo> windows_;
  unsigned long target_window_id_ = 0;
  
  AudioSource audio_source_ = AudioSource::NONE;
  bool audio_enabled_ = false;
  
  RecordStatus record_status_ = RecordStatus::IDLE;
  QString recorded_file_path_;
  
  CapturedFrame last_frame_;
  int bitrate_kbps_ = 0; // 0 = auto
  
  Stats stats_;
  
  std::map<uint32_t, std::vector<Permission>> permissions_;
  PermissionLevel default_permission_ = PermissionLevel::NONE;
  
  FrameCallback frame_callback_;
  ErrorCallback error_callback_;
  
  mutable std::mutex mutex_;
};

} // namespace hh::client

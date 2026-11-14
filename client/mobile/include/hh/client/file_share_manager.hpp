// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace hh::client {

// Datei-Share: Upload/Download, Verschlüsselung, Viren-Scan
class FileShareManager : public QObject {
  Q_OBJECT

public:
  explicit FileShareManager(QObject* parent = nullptr);
  ~FileShareManager() override = default;

  // Nicht kopierbar, bewegbar
  FileShareManager(const FileShareManager&) = delete;
  FileShareManager& operator=(const FileShareManager&) = delete;
  FileShareManager(FileShareManager&&) noexcept;
  FileShareManager& operator=(FileShareManager&&) noexcept;

  // Datei-Status
  enum class FileStatus {
    PENDING,
    UPLOADING,
    DOWNLOADED,
    ENCRYPTED,
    SCAN_PENDING,
    SCAN_CLEAN,
    SCAN_INFECTED,
    ERROR
  };
  Q_ENUM(FileStatus)

  // Datei-Info
  struct FileInfo {
    QString id;
    QString name;
    QString path;
    qint64 size_bytes;
    QString mime_type;
    uint32_t channel_id;
    uint32_t uploader_id;
    QDateTime uploaded_at;
    FileStatus status;
    QString encryption_key; // AES-256-GCM key (hex)
    QString sha256_hash;
    double progress; // 0.0-1.0
  };

  // Upload
  struct UploadRequest {
    QString file_path;
    QString file_name;
    uint32_t channel_id;
    bool encrypt;
    bool scan_virus;
  };

  struct UploadResult {
    bool success;
    QString file_id;
    QString error_message;
    qint64 bytes_uploaded;
    double duration_seconds;
  };

  UploadResult upload_file(const UploadRequest& request);
  bool cancel_upload(const QString& file_id);

  // Download
  struct DownloadRequest {
    QString file_id;
    QString save_path;
    bool decrypt;
  };

  struct DownloadResult {
    bool success;
    QString saved_path;
    QString error_message;
    qint64 bytes_downloaded;
    double duration_seconds;
  };

  DownloadResult download_file(const DownloadRequest& request);
  bool cancel_download(const QString& file_id);

  // Datei-Verwaltung
  std::vector<FileInfo> get_channel_files(uint32_t channel_id) const;
  std::vector<FileInfo> get_all_files() const;
  FileInfo* find_file(const QString& file_id);
  const FileInfo* find_file(const QString& file_id) const;
  bool delete_file(const QString& file_id);

  // Vorschau
  enum class PreviewType {
    NONE,
    IMAGE,
    VIDEO,
    AUDIO,
    PDF
  };
  Q_ENUM(PreviewType)

  PreviewType get_preview_type(const QString& mime_type) const;
  QByteArray get_preview_image(const QString& file_id, int width = 200, int height = 200);

  // Verschlüsselung
  enum class EncryptionStatus {
    NOT_ENCRYPTED,
    ENCRYPTED,
    DECRYPTED,
    ERROR
  };
  Q_ENUM(EncryptionStatus)

  EncryptionStatus encrypt_file(const QString& file_id, const QByteArray& key);
  EncryptionStatus decrypt_file(const QString& file_id, const QByteArray& key);
  QByteArray generate_encryption_key() const;

  // Hash-Prüfung
  bool verify_file_hash(const QString& file_id, const QString& expected_hash);
  QString calculate_file_hash(const QString& file_path) const;

  // Bandbreitenlimit
  void set_upload_bandwidth_limit(int limit_kbps); // 0 = kein Limit
  void set_download_bandwidth_limit(int limit_kbps);
  int upload_bandwidth_limit() const;
  int download_bandwidth_limit() const;

  // Max. Dateigröße
  void set_max_file_size_mb(int max_size_mb);
  int max_file_size_mb() const;

  // Statistik
  struct Stats {
    size_t files_uploaded;
    size_t files_downloaded;
    size_t files_failed;
    qint64 total_upload_bytes;
    qint64 total_download_bytes;
    size_t viruses_detected;
    size_t encryption_errors;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using UploadCallback = std::function<void(const QString& file_id, double progress, qint64 bytes)>;
  using DownloadCallback = std::function<void(const QString& file_id, double progress, qint64 bytes)>;
  using ErrorCallback = std::function<void(const QString& file_id, const QString& error)>;

  void set_upload_callback(UploadCallback callback);
  void set_download_callback(DownloadCallback callback);
  void set_error_callback(ErrorCallback callback);

signals:
  void upload_started(const QString& file_id);
  void upload_progress(const QString& file_id, double progress, qint64 bytes);
  void upload_completed(const QString& file_id);
  void upload_failed(const QString& file_id, const QString& error);
  void download_started(const QString& file_id);
  void download_progress(const QString& file_id, double progress, qint64 bytes);
  void download_completed(const QString& file_id);
  void download_failed(const QString& file_id, const QString& error);
  void file_deleted(const QString& file_id);
  void virus_detected(const QString& file_id);
  void error_occurred(const QString& error);

private slots:
  void on_upload_progress(qint64 bytes_sent, qint64 bytes_total);
  void on_download_progress(qint64 bytes_received, qint64 bytes_total);

private:
  std::map<QString, FileInfo> files_;
  std::map<QString, UploadRequest> active_uploads_;
  std::map<QString, DownloadRequest> active_downloads_;
  
  int upload_bandwidth_limit_kbps_ = 0; // 0 = kein Limit
  int download_bandwidth_limit_kbps_ = 0;
  int max_file_size_mb_ = 512;
  
  Stats stats_;
  
  UploadCallback upload_callback_;
  DownloadCallback download_callback_;
  ErrorCallback error_callback_;
  
  mutable std::mutex mutex_;
};

} // namespace hh::client

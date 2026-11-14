// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file_share_manager.hpp"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMimeDatabase>

namespace hh::client {

FileShareManager::FileShareManager(QObject* parent)
  : QObject(parent) {
}

FileShareManager::FileShareManager(FileShareManager&& other) noexcept
  : QObject(other.parent())
  , files_(std::move(other.files_))
  , active_uploads_(std::move(other.active_uploads_))
  , active_downloads_(std::move(other.active_downloads_))
  , upload_bandwidth_limit_kbps_(other.upload_bandwidth_limit_kbps_)
  , download_bandwidth_limit_kbps_(other.download_bandwidth_limit_kbps_)
  , max_file_size_mb_(other.max_file_size_mb_)
  , stats_(other.stats_)
  , upload_callback_(std::move(other.upload_callback_))
  , download_callback_(std::move(other.download_callback_))
  , error_callback_(std::move(other.error_callback_)) {
  
  other.upload_bandwidth_limit_kbps_ = 0;
  other.download_bandwidth_limit_kbps_ = 0;
  other.max_file_size_mb_ = 512;
  other.stats_ = Stats{};
}

FileShareManager& FileShareManager::operator=(FileShareManager&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    files_ = std::move(other.files_);
    active_uploads_ = std::move(other.active_uploads_);
    active_downloads_ = std::move(other.active_downloads_);
    upload_bandwidth_limit_kbps_ = other.upload_bandwidth_limit_kbps_;
    download_bandwidth_limit_kbps_ = other.download_bandwidth_limit_kbps_;
    max_file_size_mb_ = other.max_file_size_mb_;
    stats_ = other.stats_;
    upload_callback_ = std::move(other.upload_callback_);
    download_callback_ = std::move(other.download_callback_);
    error_callback_ = std::move(other.error_callback_);
    other.upload_bandwidth_limit_kbps_ = 0;
    other.download_bandwidth_limit_kbps_ = 0;
    other.max_file_size_mb_ = 512;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Upload ───────────────────────────────────────────────────────────

FileShareManager::UploadResult FileShareManager::upload_file(const UploadRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Datei prüfen
  QFile file(request.file_path);
  if (!file.exists()) {
    UploadResult result;
    result.success = false;
    result.error_message = "Datei existiert nicht";
    stats_.files_failed++;
    return result;
  }
  
  // Dateigröße prüfen
  QFileInfo info(file);
  if (info.size() > static_cast<qint64>(max_file_size_mb_) * 1024 * 1024) {
    UploadResult result;
    result.success = false;
    result.error_message = "Datei zu groß (max " + QString::number(max_file_size_mb_) + " MB)";
    stats_.files_failed++;
    return result;
  }
  
  // Datei-Info erstellen
  FileInfo file_info;
  file_info.id = QUuid::createUuid().toString().mid(1, 8);
  file_info.name = request.file_name.isEmpty() ? info.fileName() : request.file_name;
  file_info.path = request.file_path;
  file_info.size_bytes = info.size();
  
  QMimeDatabase mimeDb;
  file_info.mime_type = mimeDb.mimeTypeForFile(info).name();
  file_info.channel_id = request.channel_id;
  file_info.uploaded_at = QDateTime::currentDateTime();
  file_info.status = request.encrypt ? FileStatus::ENCRYPTED : FileStatus::PENDING;
  file_info.progress = 0.0;
  
  // Hash berechnen
  file_info.sha256_hash = calculate_file_hash(request.file_path);
  
  // Verschlüsselung
  if (request.encrypt) {
    QByteArray key = generate_encryption_key();
    file_info.encryption_key = key.toHex();
  }
  
  files_[file_info.id] = file_info;
  active_uploads_[file_info.id] = request;
  
  emit upload_started(file_info.id);
  
  if (upload_callback_) {
    upload_callback_(file_info.id, 0.0, 0);
  }
  
  // Simuliere Upload-Fortschritt
  stats_.files_uploaded++;
  
  UploadResult result;
  result.success = true;
  result.file_id = file_info.id;
  result.bytes_uploaded = file_info.size_bytes;
  result.duration_seconds = 1.0; // Simuliert
  
  return result;
}

bool FileShareManager::cancel_upload(const QString& file_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = active_uploads_.find(file_id);
  if (it != active_uploads_.end()) {
    active_uploads_.erase(it);
    return true;
  }
  return false;
}

// ── Download ─────────────────────────────────────────────────────────

FileShareManager::DownloadResult FileShareManager::download_file(const DownloadRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(request.file_id);
  if (it == files_.end()) {
    DownloadResult result;
    result.success = false;
    result.error_message = "Datei nicht gefunden";
    stats_.files_failed++;
    return result;
  }
  
  active_downloads_[request.file_id] = request;
  
  emit download_started(request.file_id);
  
  if (download_callback_) {
    download_callback_(request.file_id, 0.0, 0);
  }
  
  // Simuliere Download-Fortschritt
  stats_.files_downloaded++;
  
  DownloadResult result;
  result.success = true;
  result.saved_path = request.save_path;
  result.bytes_downloaded = it->second.size_bytes;
  result.duration_seconds = 1.0; // Simuliert
  
  return result;
}

bool FileShareManager::cancel_download(const QString& file_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = active_downloads_.find(file_id);
  if (it != active_downloads_.end()) {
    active_downloads_.erase(it);
    return true;
  }
  return false;
}

// ── Datei-Verwaltung ─────────────────────────────────────────────────

std::vector<FileShareManager::FileInfo> FileShareManager::get_channel_files(uint32_t channel_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<FileInfo> result;
  for (const auto& pair : files_) {
    if (pair.second.channel_id == channel_id) {
      result.push_back(pair.second);
    }
  }
  return result;
}

std::vector<FileShareManager::FileInfo> FileShareManager::get_all_files() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<FileInfo> result;
  for (const auto& pair : files_) {
    result.push_back(pair.second);
  }
  return result;
}

FileShareManager::FileInfo* FileShareManager::find_file(const QString& file_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(file_id);
  if (it != files_.end()) {
    return &it->second;
  }
  return nullptr;
}

const FileShareManager::FileInfo* FileShareManager::find_file(const QString& file_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(file_id);
  if (it != files_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool FileShareManager::delete_file(const QString& file_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(file_id);
  if (it != files_.end()) {
    files_.erase(it);
    emit file_deleted(file_id);
    return true;
  }
  return false;
}

// ── Vorschau ─────────────────────────────────────────────────────────

FileShareManager::PreviewType FileShareManager::get_preview_type(const QString& mime_type) const {
  if (mime_type.startsWith("image/")) {
    return PreviewType::IMAGE;
  } else if (mime_type.startsWith("video/")) {
    return PreviewType::VIDEO;
  } else if (mime_type.startsWith("audio/")) {
    return PreviewType::AUDIO;
  } else if (mime_type == "application/pdf") {
    return PreviewType::PDF;
  }
  return PreviewType::NONE;
}

QByteArray FileShareManager::get_preview_image(const QString& file_id, int width, int height) {
  // In einer echten Implementierung würde hier das Bild skaliert werden
  return QByteArray();
}

// ── Verschlüsselung ──────────────────────────────────────────────────

FileShareManager::EncryptionStatus FileShareManager::encrypt_file(const QString& file_id, const QByteArray& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(file_id);
  if (it == files_.end()) {
    return EncryptionStatus::ERROR;
  }
  
  // AES-256-GCM Verschlüsselung (simuliert)
  it->second.status = FileStatus::ENCRYPTED;
  it->second.encryption_key = key.toHex();
  
  return EncryptionStatus::ENCRYPTED;
}

FileShareManager::EncryptionStatus FileShareManager::decrypt_file(const QString& file_id, const QByteArray& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(file_id);
  if (it == files_.end()) {
    return EncryptionStatus::ERROR;
  }
  
  // AES-256-GCM Entschlüsselung (simuliert)
  it->second.status = FileStatus::DOWNLOADED;
  it->second.encryption_key.clear();
  
  return EncryptionStatus::DECRYPTED;
}

QByteArray FileShareManager::generate_encryption_key() const {
  QByteArray key(32, 0); // 256 Bit = 32 Byte
  for (int i = 0; i < 32; i++) {
    key[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
  }
  return key;
}

// ── Hash-Prüfung ─────────────────────────────────────────────────────

bool FileShareManager::verify_file_hash(const QString& file_id, const QString& expected_hash) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = files_.find(file_id);
  if (it == files_.end()) {
    return false;
  }
  
  return it->second.sha256_hash == expected_hash;
}

QString FileShareManager::calculate_file_hash(const QString& file_path) const {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QString();
  }
  
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(&file);
  file.close();
  
  return hash.result().toHex();
}

// ── Bandbreitenlimit ─────────────────────────────────────────────────

void FileShareManager::set_upload_bandwidth_limit(int limit_kbps) {
  std::lock_guard<std::mutex> lock(mutex_);
  upload_bandwidth_limit_kbps_ = limit_kbps;
}

void FileShareManager::set_download_bandwidth_limit(int limit_kbps) {
  std::lock_guard<std::mutex> lock(mutex_);
  download_bandwidth_limit_kbps_ = limit_kbps;
}

int FileShareManager::upload_bandwidth_limit() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return upload_bandwidth_limit_kbps_;
}

int FileShareManager::download_bandwidth_limit() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return download_bandwidth_limit_kbps_;
}

// ── Max. Dateigröße ──────────────────────────────────────────────────

void FileShareManager::set_max_file_size_mb(int max_size_mb) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_file_size_mb_ = max_size_mb;
}

int FileShareManager::max_file_size_mb() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return max_file_size_mb_;
}

// ── Statistik ────────────────────────────────────────────────────────

FileShareManager::Stats FileShareManager::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void FileShareManager::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
}

// ── Callbacks ────────────────────────────────────────────────────────

void FileShareManager::set_upload_callback(UploadCallback callback) {
  upload_callback_ = std::move(callback);
}

void FileShareManager::set_download_callback(DownloadCallback callback) {
  download_callback_ = std::move(callback);
}

void FileShareManager::set_error_callback(ErrorCallback callback) {
  error_callback_ = std::move(callback);
}

// ── Slots ────────────────────────────────────────────────────────────

void FileShareManager::on_upload_progress(qint64 bytes_sent, qint64 bytes_total) {
  double progress = static_cast<double>(bytes_sent) / bytes_total;
  
  // Callback aufrufen
  if (upload_callback_) {
    // In einer echten Implementierung würde hier die file_id übertragen
  }
}

void FileShareManager::on_download_progress(qint64 bytes_received, qint64 bytes_total) {
  double progress = static_cast<double>(bytes_received) / bytes_total;
  
  // Callback aufrufen
  if (download_callback_) {
    // In einer echten Implementierung würde hier die file_id übertragen
  }
}

} // namespace hh::client

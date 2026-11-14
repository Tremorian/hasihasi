// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protocol_version.hpp"

#include "hh/logging.hpp"

#include <algorithm>

namespace hh::server {

// ── ProtocolVersionManager ──────────────────────────────────────────

ProtocolVersionManager::ProtocolVersionManager(ProtocolVersionManager&& other) noexcept
  : min_version_(other.min_version_)
  , max_version_(other.max_version_)
  , current_version_(other.current_version_)
  , deprecated_versions_(std::move(other.deprecated_versions_))
  , beta_versions_(std::move(other.beta_versions_))
  , stats_(other.stats_)
  , version_check_callback_(std::move(other.version_check_callback_)) {
  
  other.min_version_ = MIN_VERSION;
  other.max_version_ = MAX_VERSION;
  other.current_version_ = CURRENT_VERSION;
  other.stats_ = Stats{};
}

ProtocolVersionManager& ProtocolVersionManager::operator=(ProtocolVersionManager&& other) noexcept {
  if (this != &other) {
    min_version_ = other.min_version_;
    max_version_ = other.max_version_;
    current_version_ = other.current_version_;
    deprecated_versions_ = std::move(other.deprecated_versions_);
    beta_versions_ = std::move(other.beta_versions_);
    stats_ = other.stats_;
    version_check_callback_ = std::move(other.version_check_callback_);
    other.min_version_ = MIN_VERSION;
    other.max_version_ = MAX_VERSION;
    other.current_version_ = CURRENT_VERSION;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Version prüfen ──────────────────────────────────────────────────

ProtocolVersionManager::VersionCheckResult ProtocolVersionManager::check_version(uint32_t client_version) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  stats_.version_checks++;
  
  VersionCheckResult result;
  result.success = false;
  result.negotiated_version = 0;
  result.is_deprecated = false;
  result.is_beta = false;
  
  // Version im erlaubten Bereich?
  if (client_version < min_version_ || client_version > max_version_) {
    result.error_message = "Protokollversion " + std::to_string(client_version) + 
                           " wird nicht unterstützt. Erlaubt: " + 
                           std::to_string(min_version_) + "-" + std::to_string(max_version_);
    stats_.version_rejects++;
    
    if (version_check_callback_) {
      version_check_callback_(result);
    }
    
    return result;
  }
  
  // Version erfolgreich ausgehandelt
  result.success = true;
  result.negotiated_version = client_version;
  
  // Ist die Version deprecated?
  auto it = deprecated_versions_.find(client_version);
  if (it != deprecated_versions_.end() && it->second) {
    result.is_deprecated = true;
    stats_.deprecated_accepts++;
    hh::get_logger().warn("ProtocolVersionManager: Deprecated version {} accepted", client_version);
  } else {
    stats_.version_accepts++;
  }
  
  // Ist die Version beta?
  auto betait = beta_versions_.find(client_version);
  if (betait != beta_versions_.end() && betait->second) {
    result.is_beta = true;
    stats_.beta_accepts++;
    hh::get_logger().info("ProtocolVersionManager: Beta version {} accepted", client_version);
  }
  
  if (version_check_callback_) {
    version_check_callback_(result);
  }
  
  return result;
}

// ── Version aushandeln ──────────────────────────────────────────────

uint32_t ProtocolVersionManager::negotiate_version(uint32_t client_version) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Wenn Client-Version im erlaubten Bereich, diese verwenden
  if (client_version >= min_version_ && client_version <= max_version_) {
    return client_version;
  }
  
  // Sonst Current-Version vorschlagen
  return current_version_;
}

// ── Version-Informationen ───────────────────────────────────────────

ProtocolVersionManager::VersionInfo ProtocolVersionManager::get_current_version_info() const {
  VersionInfo info;
  info.version = current_version_;
  info.description = "HasiHasi Protokoll Version " + std::to_string(current_version_);
  
  auto it = deprecated_versions_.find(current_version_);
  info.is_deprecated = (it != deprecated_versions_.end() && it->second);
  
  auto betait = beta_versions_.find(current_version_);
  info.is_beta = (betait != beta_versions_.end() && betait->second);
  
  return info;
}

std::vector<ProtocolVersionManager::VersionInfo> ProtocolVersionManager::get_supported_versions() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<VersionInfo> versions;
  
  for (uint32_t v = min_version_; v <= max_version_; v++) {
    VersionInfo info;
    info.version = v;
    info.description = "HasiHasi Protokoll Version " + std::to_string(v);
    
    auto it = deprecated_versions_.find(v);
    info.is_deprecated = (it != deprecated_versions_.end() && it->second);
    
    auto betait = beta_versions_.find(v);
    info.is_beta = (betait != beta_versions_.end() && betait->second);
    
    versions.push_back(info);
  }
  
  return versions;
}

bool ProtocolVersionManager::is_version_deprecated(uint32_t version) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = deprecated_versions_.find(version);
  return it != deprecated_versions_.end() && it->second;
}

bool ProtocolVersionManager::is_version_beta(uint32_t version) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = beta_versions_.find(version);
  return it != beta_versions_.end() && it->second;
}

// ── Konfiguration ───────────────────────────────────────────────────

void ProtocolVersionManager::set_min_version(uint32_t version) {
  std::lock_guard<std::mutex> lock(mutex_);
  min_version_ = version;
}

void ProtocolVersionManager::set_max_version(uint32_t version) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_version_ = version;
}

void ProtocolVersionManager::set_current_version(uint32_t version) {
  std::lock_guard<std::mutex> lock(mutex_);
  current_version_ = version;
}

// ── Statistik ───────────────────────────────────────────────────────

ProtocolVersionManager::Stats ProtocolVersionManager::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void ProtocolVersionManager::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
}

// ── Callbacks ───────────────────────────────────────────────────────

void ProtocolVersionManager::set_version_check_callback(VersionCheckCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  version_check_callback_ = std::move(callback);
}

} // namespace hh::server

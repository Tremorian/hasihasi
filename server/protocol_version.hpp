// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace hh::server {

// Protokollversionierung für den Handshake
class ProtocolVersionManager {
public:
  // Unterstützte Protokollversionen
  static constexpr uint32_t CURRENT_VERSION = 1;
  static constexpr uint32_t MIN_VERSION = 1;
  static constexpr uint32_t MAX_VERSION = 1;

  struct VersionInfo {
    uint32_t version;
    std::string description;
    bool is_deprecated;
    bool is_beta;
  };

  struct VersionCheckResult {
    bool success;
    uint32_t negotiated_version;
    std::string error_message;
    bool is_deprecated;
    bool is_beta;
  };

  ProtocolVersionManager() = default;
  ~ProtocolVersionManager() = default;

  // Nicht kopierbar, bewegbar
  ProtocolVersionManager(const ProtocolVersionManager&) = delete;
  ProtocolVersionManager& operator=(const ProtocolVersionManager&) = delete;
  ProtocolVersionManager(ProtocolVersionManager&&) noexcept;
  ProtocolVersionManager& operator=(ProtocolVersionManager&&) noexcept;

  // Version prüfen
  VersionCheckResult check_version(uint32_t client_version) const;

  // Version aushandeln
  uint32_t negotiate_version(uint32_t client_version) const;

  // Version-Informationen
  VersionInfo get_current_version_info() const;
  std::vector<VersionInfo> get_supported_versions() const;

  // Deprecated-Versionen
  bool is_version_deprecated(uint32_t version) const;
  bool is_version_beta(uint32_t version) const;

  // Konfiguration
  void set_min_version(uint32_t version);
  void set_max_version(uint32_t version);
  void set_current_version(uint32_t version);

  // Statistik
  struct Stats {
    size_t version_checks;
    size_t version_accepts;
    size_t version_rejects;
    size_t deprecated_accepts;
    size_t beta_accepts;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using VersionCheckCallback = std::function<void(const VersionCheckResult& result)>;
  void set_version_check_callback(VersionCheckCallback callback);

private:
  mutable std::mutex mutex_;
  uint32_t min_version_ = MIN_VERSION;
  uint32_t max_version_ = MAX_VERSION;
  uint32_t current_version_ = CURRENT_VERSION;
  
  // Deprecated-Versionen
  std::map<uint32_t, bool> deprecated_versions_;
  std::map<uint32_t, bool> beta_versions_;
  
  Stats stats_;
  VersionCheckCallback version_check_callback_;
};

} // namespace hh::server

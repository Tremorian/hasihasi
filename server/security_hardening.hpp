// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace hh::server {

// Security-Hardening für den HasiHasi-Server
class SecurityHardening {
public:
  struct SecurityConfig {
    // Verschlüsselung
    bool enable_encryption = true;
    std::string encryption_algorithm = "AES-256-GCM";
    size_t key_length = 256;
    
    // Rate-Limiting
    int max_requests_per_second = 100;
    int max_connections_per_ip = 10;
    int max_messages_per_second = 50;
    
    // Sicherheits-Header
    bool enable_hsts = true;
    bool enable_csp = true;
    bool enable_x_frame_options = true;
    bool enable_x_content_type_options = true;
    
    // Audit-Logging
    bool enable_audit_log = true;
    std::string audit_log_path = "/var/log/hasihasi/audit.log";
    
    // Sicherheits-Scans
    bool enable_security_scan = true;
    int scan_interval_seconds = 3600;
  };

  struct SecurityEvent {
    std::string event_type;
    std::string severity; // low, medium, high, critical
    std::string description;
    std::string source_ip;
    uint32_t user_id = 0;
    double timestamp = 0.0;
  };

  SecurityHardening() = default;
  ~SecurityHardening() = default;

  // Nicht kopierbar, bewegbar
  SecurityHardening(const SecurityHardening&) = delete;
  SecurityHardening& operator=(const SecurityHardening&) = delete;
  SecurityHardening(SecurityHardening&&) noexcept;
  SecurityHardening& operator=(SecurityHardening&&) noexcept;

  // Konfiguration
  void set_config(const SecurityConfig& config);
  SecurityConfig get_config() const;

  // Verschlüsselung
  bool initialize_encryption();
  std::string encrypt(const std::string& plaintext);
  std::string decrypt(const std::string& ciphertext);
  bool rotate_keys();

  // Rate-Limiting
  bool is_rate_limited(const std::string& ip);
  bool is_connection_limit_exceeded(const std::string& ip);
  bool is_message_limit_exceeded(uint32_t user_id);
  void reset_rate_limits();

  // Sicherheits-Header
  std::string get_hsts_header() const;
  std::string get_csp_header() const;
  std::string get_x_frame_options_header() const;
  std::string get_x_content_type_options_header() const;

  // Audit-Logging
  void log_security_event(const SecurityEvent& event);
  std::vector<SecurityEvent> get_security_events(int count = 100) const;
  std::vector<SecurityEvent> get_security_events_by_type(const std::string& event_type) const;
  std::vector<SecurityEvent> get_security_events_by_severity(const std::string& severity) const;
  void clear_security_events();

  // Sicherheits-Scans
  bool start_security_scan();
  bool is_security_scan_running() const { return is_security_scan_running_; }
  void stop_security_scan();

  // IP-Filterung
  bool add_ip_block(const std::string& ip);
  bool remove_ip_block(const std::string& ip);
  bool is_ip_blocked(const std::string& ip) const;
  std::vector<std::string> get_blocked_ips() const;

  // Whitelist
  bool add_ip_whitelist(const std::string& ip);
  bool remove_ip_whitelist(const std::string& ip);
  bool is_ip_whitelisted(const std::string& ip) const;
  std::vector<std::string> get_whitelisted_ips() const;

  // Callbacks
  using SecurityEventCallback = std::function<void(const SecurityEvent& event)>;
  
  void set_security_event_callback(SecurityEventCallback callback);

  // Statistiken
  struct SecurityStats {
    // Verschlüsselung
    size_t encrypted_messages = 0;
    size_t decrypted_messages = 0;
    size_t key_rotations = 0;
    
    // Rate-Limiting
    size_t rate_limited_requests = 0;
    size_t connection_limits_exceeded = 0;
    size_t message_limits_exceeded = 0;
    
    // Audit-Logging
    size_t total_security_events = 0;
    size_t critical_events = 0;
    size_t high_events = 0;
    size_t medium_events = 0;
    size_t low_events = 0;
    
    // IP-Filterung
    size_t blocked_ips = 0;
    size_t whitelisted_ips = 0;
  };

  SecurityStats get_stats() const;
  void reset_stats();

private:
  // Scan-Thread
  void scan_thread_func();
  
  // Einstellungen
  SecurityConfig config_;
  bool is_security_scan_running_ = false;
  
  // Verschlüsselung
  std::string encryption_key_;
  mutable std::mutex encryption_mutex_;
  
  // Rate-Limiting
  mutable std::mutex rate_limit_mutex_;
  std::unordered_map<std::string, std::vector<double>> request_timestamps_;
  std::unordered_map<std::string, std::vector<double>> connection_timestamps_;
  std::unordered_map<uint32_t, std::vector<double>> message_timestamps_;
  
  // Audit-Logging
  mutable std::mutex audit_mutex_;
  std::vector<SecurityEvent> security_events_;
  
  // IP-Filterung
  mutable std::mutex ip_mutex_;
  std::vector<std::string> blocked_ips_;
  std::vector<std::string> whitelisted_ips_;
  
  // Callbacks
  SecurityEventCallback security_event_callback_;
  
  // Statistiken
  mutable std::atomic<size_t> encrypted_messages_{0};
  mutable std::atomic<size_t> decrypted_messages_{0};
  mutable std::atomic<size_t> key_rotations_{0};
  mutable std::atomic<size_t> rate_limited_requests_{0};
  mutable std::atomic<size_t> connection_limits_exceeded_{0};
  mutable std::atomic<size_t> message_limits_exceeded_{0};
  mutable std::atomic<size_t> critical_events_{0};
  mutable std::atomic<size_t> high_events_{0};
  mutable std::atomic<size_t> medium_events_{0};
  mutable std::atomic<size_t> low_events_{0};
  
  // Thread
  std::unique_ptr<std::thread> scan_thread_;
};

} // namespace hh::server

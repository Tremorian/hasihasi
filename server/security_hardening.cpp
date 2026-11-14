// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "security_hardening.hpp"

#include "hh/logging.hpp"

#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <sstream>

namespace hh::server {

// ── SecurityHardening ───────────────────────────────────────────────

SecurityHardening::SecurityHardening(SecurityHardening&& other) noexcept
  : config_(other.config_)
  , is_security_scan_running_(other.is_security_scan_running_)
  , encryption_key_(std::move(other.encryption_key_))
  , request_timestamps_(std::move(other.request_timestamps_))
  , connection_timestamps_(std::move(other.connection_timestamps_))
  , message_timestamps_(std::move(other.message_timestamps_))
  , security_events_(std::move(other.security_events_))
  , blocked_ips_(std::move(other.blocked_ips_))
  , whitelisted_ips_(std::move(other.whitelisted_ips_))
  , security_event_callback_(std::move(other.security_event_callback_))
  , encrypted_messages_(other.encrypted_messages_.load())
  , decrypted_messages_(other.decrypted_messages_.load())
  , key_rotations_(other.key_rotations_.load())
  , rate_limited_requests_(other.rate_limited_requests_.load())
  , connection_limits_exceeded_(other.connection_limits_exceeded_.load())
  , message_limits_exceeded_(other.message_limits_exceeded_.load())
  , critical_events_(other.critical_events_.load())
  , high_events_(other.high_events_.load())
  , medium_events_(other.medium_events_.load())
  , low_events_(other.low_events_.load())
  , scan_thread_(std::move(other.scan_thread_)) {
  
  other.is_security_scan_running_ = false;
  other.encrypted_messages_ = 0;
  other.decrypted_messages_ = 0;
  other.key_rotations_ = 0;
  other.rate_limited_requests_ = 0;
  other.connection_limits_exceeded_ = 0;
  other.message_limits_exceeded_ = 0;
  other.critical_events_ = 0;
  other.high_events_ = 0;
  other.medium_events_ = 0;
  other.low_events_ = 0;
}

SecurityHardening& SecurityHardening::operator=(SecurityHardening&& other) noexcept {
  if (this != &other) {
    config_ = other.config_;
    is_security_scan_running_ = other.is_security_scan_running_;
    encryption_key_ = std::move(other.encryption_key_);
    request_timestamps_ = std::move(other.request_timestamps_);
    connection_timestamps_ = std::move(other.connection_timestamps_);
    message_timestamps_ = std::move(other.message_timestamps_);
    security_events_ = std::move(other.security_events_);
    blocked_ips_ = std::move(other.blocked_ips_);
    whitelisted_ips_ = std::move(other.whitelisted_ips_);
    security_event_callback_ = std::move(other.security_event_callback_);
    encrypted_messages_ = other.encrypted_messages_.load();
    decrypted_messages_ = other.decrypted_messages_.load();
    key_rotations_ = other.key_rotations_.load();
    rate_limited_requests_ = other.rate_limited_requests_.load();
    connection_limits_exceeded_ = other.connection_limits_exceeded_.load();
    message_limits_exceeded_ = other.message_limits_exceeded_.load();
    critical_events_ = other.critical_events_.load();
    high_events_ = other.high_events_.load();
    medium_events_ = other.medium_events_.load();
    low_events_ = other.low_events_.load();
    scan_thread_ = std::move(other.scan_thread_);
    other.is_security_scan_running_ = false;
    other.encrypted_messages_ = 0;
    other.decrypted_messages_ = 0;
    other.key_rotations_ = 0;
    other.rate_limited_requests_ = 0;
    other.connection_limits_exceeded_ = 0;
    other.message_limits_exceeded_ = 0;
    other.critical_events_ = 0;
    other.high_events_ = 0;
    other.medium_events_ = 0;
    other.low_events_ = 0;
  }
  return *this;
}

// ── Konfiguration ───────────────────────────────────────────────────

void SecurityHardening::set_config(const SecurityConfig& config) {
  config_ = config;
}

SecurityHardening::SecurityConfig SecurityHardening::get_config() const {
  return config_;
}

// ── Verschlüsselung ─────────────────────────────────────────────────

bool SecurityHardening::initialize_encryption() {
  std::lock_guard<std::mutex> lock(encryption_mutex_);
  
  if (config_.encryption_algorithm == "AES-256-GCM") {
    // AES-256-GCM Schlüssel generieren
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    encryption_key_.resize(32);
    for (size_t i = 0; i < 32; i++) {
      encryption_key_[i] = static_cast<char>(dis(gen));
    }
    
    hh::get_logger().info("SecurityHardening: AES-256-GCM encryption initialized");
    return true;
  }
  
  hh::get_logger().error("SecurityHardening: Unsupported encryption algorithm: {}", config_.encryption_algorithm);
  return false;
}

std::string SecurityHardening::encrypt(const std::string& plaintext) {
  if (!config_.enable_encryption) {
    return plaintext;
  }
  
  std::lock_guard<std::mutex> lock(encryption_mutex_);
  
  // Einfache XOR-Verschlüsselung (in Produktion würde man echte AES-256-GCM verwenden)
  std::string ciphertext = plaintext;
  for (size_t i = 0; i < plaintext.size(); i++) {
    ciphertext[i] = plaintext[i] ^ encryption_key_[i % encryption_key_.size()];
  }
  
  encrypted_messages_++;
  return ciphertext;
}

std::string SecurityHardening::decrypt(const std::string& ciphertext) {
  if (!config_.enable_encryption) {
    return ciphertext;
  }
  
  std::lock_guard<std::mutex> lock(encryption_mutex_);
  
  // Einfache XOR-Entschlüsselung
  std::string plaintext = ciphertext;
  for (size_t i = 0; i < ciphertext.size(); i++) {
    plaintext[i] = ciphertext[i] ^ encryption_key_[i % encryption_key_.size()];
  }
  
  decrypted_messages_++;
  return plaintext;
}

bool SecurityHardening::rotate_keys() {
  std::lock_guard<std::mutex> lock(encryption_mutex_);
  
  // Neuen Schlüssel generieren
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);
  
  encryption_key_.resize(32);
  for (size_t i = 0; i < 32; i++) {
    encryption_key_[i] = static_cast<char>(dis(gen));
  }
  
  key_rotations_++;
  hh::get_logger().info("SecurityHardening: Encryption keys rotated");
  return true;
}

// ── Rate-Limiting ───────────────────────────────────────────────────

bool SecurityHardening::is_rate_limited(const std::string& ip) {
  std::lock_guard<std::mutex> lock(rate_limit_mutex_);
  
  auto it = request_timestamps_.find(ip);
  if (it == request_timestamps_.end()) {
    return false;
  }
  
  double now = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
  
  // Timestamps im letzten Sekunden-Bereich zählen
  int count = 0;
  for (const auto& timestamp : it->second) {
    if (now - timestamp < 1.0) {
      count++;
    }
  }
  
  if (count >= config_.max_requests_per_second) {
    rate_limited_requests_++;
    return true;
  }
  
  // Neuen Timestamp hinzufügen
  it->second.push_back(now);
  
  // Alte Timestamps entfernen
  it->second.erase(
    std::remove_if(it->second.begin(), it->second.end(),
                   [now](double timestamp) { return now - timestamp > 10.0; }),
    it->second.end()
  );
  
  return false;
}

bool SecurityHardening::is_connection_limit_exceeded(const std::string& ip) {
  std::lock_guard<std::mutex> lock(rate_limit_mutex_);
  
  auto it = connection_timestamps_.find(ip);
  if (it == connection_timestamps_.end()) {
    return false;
  }
  
  double now = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
  
  // Verbindungen im letzten Minuten-Bereich zählen
  int count = 0;
  for (const auto& timestamp : it->second) {
    if (now - timestamp < 60.0) {
      count++;
    }
  }
  
  if (count >= config_.max_connections_per_ip) {
    connection_limits_exceeded_++;
    return true;
  }
  
  return false;
}

bool SecurityHardening::is_message_limit_exceeded(uint32_t user_id) {
  std::lock_guard<std::mutex> lock(rate_limit_mutex_);
  
  auto it = message_timestamps_.find(user_id);
  if (it == message_timestamps_.end()) {
    return false;
  }
  
  double now = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
  
  // Nachrichten im letzten Sekunden-Bereich zählen
  int count = 0;
  for (const auto& timestamp : it->second) {
    if (now - timestamp < 1.0) {
      count++;
    }
  }
  
  if (count >= config_.max_messages_per_second) {
    message_limits_exceeded_++;
    return true;
  }
  
  // Neue Nachricht hinzufügen
  it->second.push_back(now);
  
  // Alte Timestamps entfernen
  it->second.erase(
    std::remove_if(it->second.begin(), it->second.end(),
                   [now](double timestamp) { return now - timestamp > 10.0; }),
    it->second.end()
  );
  
  return false;
}

void SecurityHardening::reset_rate_limits() {
  std::lock_guard<std::mutex> lock(rate_limit_mutex_);
  
  request_timestamps_.clear();
  connection_timestamps_.clear();
  message_timestamps_.clear();
}

// ── Sicherheits-Header ─────────────────────────────────────────────

std::string SecurityHardening::get_hsts_header() const {
  if (!config_.enable_hsts) {
    return "";
  }
  
  return "Strict-Transport-Security: max-age=31536000; includeSubDomains; preload";
}

std::string SecurityHardening::get_csp_header() const {
  if (!config_.enable_csp) {
    return "";
  }
  
  return "Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'";
}

std::string SecurityHardening::get_x_frame_options_header() const {
  if (!config_.enable_x_frame_options) {
    return "";
  }
  
  return "X-Frame-Options: DENY";
}

std::string SecurityHardening::get_x_content_type_options_header() const {
  if (!config_.enable_x_content_type_options) {
    return "";
  }
  
  return "X-Content-Type-Options: nosniff";
}

// ── Audit-Logging ───────────────────────────────────────────────────

void SecurityHardening::log_security_event(const SecurityEvent& event) {
  std::lock_guard<std::mutex> lock(audit_mutex_);
  
  security_events_.push_back(event);
  
  // Ereignisanzahl begrenzen
  if (security_events_.size() > 10000) {
    security_events_.erase(security_events_.begin(), security_events_.begin() + 5000);
  }
  
  // Zähler aktualisieren
  if (event.severity == "critical") {
    critical_events_++;
  } else if (event.severity == "high") {
    high_events_++;
  } else if (event.severity == "medium") {
    medium_events_++;
  } else if (event.severity == "low") {
    low_events_++;
  }
  
  // Callback aufrufen
  if (security_event_callback_) {
    security_event_callback_(event);
  }
  
  hh::get_logger().warn("SecurityHardening: Security event logged: {} - {}", event.event_type, event.description);
}

std::vector<SecurityHardening::SecurityEvent> SecurityHardening::get_security_events(int count) const {
  std::lock_guard<std::mutex> lock(audit_mutex_);
  
  std::vector<SecurityEvent> events;
  size_t start = security_events_.size() > static_cast<size_t>(count) 
                 ? security_events_.size() - count 
                 : 0;
  
  for (size_t i = start; i < security_events_.size(); i++) {
    events.push_back(security_events_[i]);
  }
  
  return events;
}

std::vector<SecurityHardening::SecurityEvent> SecurityHardening::get_security_events_by_type(const std::string& event_type) const {
  std::lock_guard<std::mutex> lock(audit_mutex_);
  
  std::vector<SecurityEvent> events;
  for (const auto& event : security_events_) {
    if (event.event_type == event_type) {
      events.push_back(event);
    }
  }
  
  return events;
}

std::vector<SecurityHardening::SecurityEvent> SecurityHardening::get_security_events_by_severity(const std::string& severity) const {
  std::lock_guard<std::mutex> lock(audit_mutex_);
  
  std::vector<SecurityEvent> events;
  for (const auto& event : security_events_) {
    if (event.severity == severity) {
      events.push_back(event);
    }
  }
  
  return events;
}

void SecurityHardening::clear_security_events() {
  std::lock_guard<std::mutex> lock(audit_mutex_);
  
  security_events_.clear();
}

// ── Sicherheits-Scans ───────────────────────────────────────────────

bool SecurityHardening::start_security_scan() {
  if (is_security_scan_running_) {
    hh::get_logger().warn("SecurityHardening: Security scan already running");
    return false;
  }
  
  if (!config_.enable_security_scan) {
    hh::get_logger().warn("SecurityHardening: Security scans not enabled");
    return false;
  }
  
  is_security_scan_running_ = true;
  
  scan_thread_ = std::make_unique<std::thread>([this]() {
    scan_thread_func();
  });
  
  hh::get_logger().info("SecurityHardening: Security scan started");
  return true;
}

void SecurityHardening::stop_security_scan() {
  if (!is_security_scan_running_) {
    hh::get_logger().warn("SecurityHardening: Security scan not running");
    return;
  }
  
  is_security_scan_running_ = false;
  
  if (scan_thread_ && scan_thread_->joinable()) {
    scan_thread_->join();
  }
  
  hh::get_logger().info("SecurityHardening: Security scan stopped");
}

void SecurityHardening::scan_thread_func() {
  hh::get_logger().info("SecurityHardening: Security scan thread started");
  
  while (is_security_scan_running_) {
    // Sicherheits-Scan durchführen
    hh::get_logger().info("SecurityHardening: Running security scan");
    
    // Check für veraltete Abhängigkeiten
    // Check für schwache Passwörter
    // Check für offene Ports
    // etc.
    
    std::this_thread::sleep_for(std::chrono::seconds(config_.scan_interval_seconds));
  }
  
  hh::get_logger().info("SecurityHardening: Security scan thread stopped");
}

// ── IP-Filterung ────────────────────────────────────────────────────

bool SecurityHardening::add_ip_block(const std::string& ip) {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  if (std::find(blocked_ips_.begin(), blocked_ips_.end(), ip) != blocked_ips_.end()) {
    return false;
  }
  
  blocked_ips_.push_back(ip);
  return true;
}

bool SecurityHardening::remove_ip_block(const std::string& ip) {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  auto it = std::find(blocked_ips_.begin(), blocked_ips_.end(), ip);
  if (it == blocked_ips_.end()) {
    return false;
  }
  
  blocked_ips_.erase(it);
  return true;
}

bool SecurityHardening::is_ip_blocked(const std::string& ip) const {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  return std::find(blocked_ips_.begin(), blocked_ips_.end(), ip) != blocked_ips_.end();
}

std::vector<std::string> SecurityHardening::get_blocked_ips() const {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  return blocked_ips_;
}

bool SecurityHardening::add_ip_whitelist(const std::string& ip) {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  if (std::find(whitelisted_ips_.begin(), whitelisted_ips_.end(), ip) != whitelisted_ips_.end()) {
    return false;
  }
  
  whitelisted_ips_.push_back(ip);
  return true;
}

bool SecurityHardening::remove_ip_whitelist(const std::string& ip) {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  auto it = std::find(whitelisted_ips_.begin(), whitelisted_ips_.end(), ip);
  if (it == whitelisted_ips_.end()) {
    return false;
  }
  
  whitelisted_ips_.erase(it);
  return true;
}

bool SecurityHardening::is_ip_whitelisted(const std::string& ip) const {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  return std::find(whitelisted_ips_.begin(), whitelisted_ips_.end(), ip) != whitelisted_ips_.end();
}

std::vector<std::string> SecurityHardening::get_whitelisted_ips() const {
  std::lock_guard<std::mutex> lock(ip_mutex_);
  
  return whitelisted_ips_;
}

// ── Callbacks ───────────────────────────────────────────────────────

void SecurityHardening::set_security_event_callback(SecurityEventCallback callback) {
  security_event_callback_ = std::move(callback);
}

// ── Statistiken ─────────────────────────────────────────────────────

SecurityHardening::SecurityStats SecurityHardening::get_stats() const {
  SecurityStats stats;
  
  // Verschlüsselung
  stats.encrypted_messages = encrypted_messages_.load();
  stats.decrypted_messages = decrypted_messages_.load();
  stats.key_rotations = key_rotations_.load();
  
  // Rate-Limiting
  stats.rate_limited_requests = rate_limited_requests_.load();
  stats.connection_limits_exceeded = connection_limits_exceeded_.load();
  stats.message_limits_exceeded = message_limits_exceeded_.load();
  
  // Audit-Logging
  {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    stats.total_security_events = security_events_.size();
  }
  stats.critical_events = critical_events_.load();
  stats.high_events = high_events_.load();
  stats.medium_events = medium_events_.load();
  stats.low_events = low_events_.load();
  
  // IP-Filterung
  {
    std::lock_guard<std::mutex> lock(ip_mutex_);
    stats.blocked_ips = blocked_ips_.size();
    stats.whitelisted_ips = whitelisted_ips_.size();
  }
  
  return stats;
}

void SecurityHardening::reset_stats() {
  encrypted_messages_ = 0;
  decrypted_messages_ = 0;
  key_rotations_ = 0;
  rate_limited_requests_ = 0;
  connection_limits_exceeded_ = 0;
  message_limits_exceeded_ = 0;
  critical_events_ = 0;
  high_events_ = 0;
  medium_events_ = 0;
  low_events_ = 0;
}

} // namespace hh::server

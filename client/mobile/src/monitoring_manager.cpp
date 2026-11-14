// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitoring_manager.hpp"

#include <QRandomGenerator>
#include <QUuid>

namespace hh::client {

MonitoringManager::MonitoringManager(QObject* parent)
  : QObject(parent) {
  
  health_status_.is_healthy = true;
  health_status_.status_message = "OK";
  health_status_.uptime_ms = 0;
  health_status_.connected_users = 0;
  health_status_.active_channels = 0;
  health_status_.cpu_usage_percent = 0.0;
  health_status_.memory_usage_percent = 0.0;
  health_status_.network_latency_ms = 0.0;
  health_status_.last_check = QDateTime::currentDateTime();
}

MonitoringManager::MonitoringManager(MonitoringManager&& other) noexcept
  : QObject(other.parent())
  , prometheus_status_(other.prometheus_status_)
  , prometheus_url_(std::move(other.prometheus_url_))
  , grafana_status_(other.grafana_status_)
  , grafana_url_(std::move(other.grafana_url_))
  , grafana_api_key_(std::move(other.grafana_api_key_))
  , alerts_(std::move(other.alerts_))
  , current_alert_id_(std::move(other.current_alert_id_))
  , metrics_(std::move(other.metrics_))
  , gauge_values_(std::move(other.gauge_values_))
  , counter_values_(std::move(other.counter_values_))
  , histogram_values_(std::move(other.histogram_values_))
  , dashboards_(std::move(other.dashboards_))
  , health_status_(other.health_status_)
  , alert_rules_(std::move(other.alert_rules_))
  , stats_(other.stats_)
  , alert_callback_(std::move(other.alert_callback_))
  , health_callback_(std::move(other.health_callback_))
  , logs_(std::move(other.logs_))
  , current_log_id_(std::move(other.current_log_id_))
  , audit_logs_(std::move(other.audit_logs_))
  , current_audit_id_(std::move(other.current_audit_id_))
  , log_callback_(std::move(other.log_callback_))
  , audit_callback_(std::move(other.audit_callback_)) {
  
  other.prometheus_status_ = PrometheusStatus::DISABLED;
  other.grafana_status_ = GrafanaStatus::DISABLED;
  other.stats_ = Stats{};
}

MonitoringManager& MonitoringManager::operator=(MonitoringManager&& other) noexcept {
  if (this != &other) {
    QObject::operator=(std::move(other));
    prometheus_status_ = other.prometheus_status_;
    prometheus_url_ = std::move(other.prometheus_url_);
    grafana_status_ = other.grafana_status_;
    grafana_url_ = std::move(other.grafana_url_);
    grafana_api_key_ = std::move(other.grafana_api_key_);
    alerts_ = std::move(other.alerts_);
    current_alert_id_ = std::move(other.current_alert_id_);
    metrics_ = std::move(other.metrics_);
    gauge_values_ = std::move(other.gauge_values_);
    counter_values_ = std::move(other.counter_values_);
    histogram_values_ = std::move(other.histogram_values_);
    dashboards_ = std::move(other.dashboards_);
    health_status_ = other.health_status_;
    alert_rules_ = std::move(other.alert_rules_);
    stats_ = other.stats_;
    alert_callback_ = std::move(other.alert_callback_);
    health_callback_ = std::move(other.health_callback_);
    logs_ = std::move(other.logs_);
    current_log_id_ = std::move(other.current_log_id_);
    audit_logs_ = std::move(other.audit_logs_);
    current_audit_id_ = std::move(other.current_audit_id_);
    log_callback_ = std::move(other.log_callback_);
    audit_callback_ = std::move(other.audit_callback_);
    other.prometheus_status_ = PrometheusStatus::DISABLED;
    other.grafana_status_ = GrafanaStatus::DISABLED;
    other.stats_ = Stats{};
  }
  return *this;
}

// ── Prometheus ───────────────────────────────────────────────────────

MonitoringManager::PrometheusStatus MonitoringManager::prometheus_status() const {
  return prometheus_status_;
}

bool MonitoringManager::set_prometheus_url(const QString& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (prometheus_url_ != url) {
    prometheus_url_ = url;
    prometheus_status_ = PrometheusStatus::CONNECTED;
    emit prometheus_connected();
    return true;
  }
  return false;
}

QString MonitoringManager::prometheus_url() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return prometheus_url_;
}

// ── Grafana ──────────────────────────────────────────────────────────

MonitoringManager::GrafanaStatus MonitoringManager::grafana_status() const {
  return grafana_status_;
}

bool MonitoringManager::set_grafana_url(const QString& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (grafana_url_ != url) {
    grafana_url_ = url;
    grafana_status_ = GrafanaStatus::CONNECTED;
    emit grafana_connected();
    return true;
  }
  return false;
}

bool MonitoringManager::set_grafana_api_key(const QString& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  grafana_api_key_ = key;
  return true;
}

QString MonitoringManager::grafana_url() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return grafana_url_;
}

QString MonitoringManager::grafana_api_key() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return grafana_api_key_;
}

// ── Alerting ─────────────────────────────────────────────────────────

std::vector<MonitoringManager::Alert> MonitoringManager::get_alerts(AlertSeverity severity) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Alert> result;
  for (const auto& pair : alerts_) {
    if (pair.second.severity == severity) {
      result.push_back(pair.second);
    }
  }
  return result;
}

std::vector<MonitoringManager::Alert> MonitoringManager::get_active_alerts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Alert> result;
  for (const auto& pair : alerts_) {
    if (!pair.second.is_resolved) {
      result.push_back(pair.second);
    }
  }
  return result;
}

bool MonitoringManager::resolve_alert(const QString& alert_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = alerts_.find(alert_id);
  if (it != alerts_.end()) {
    it->second.is_resolved = true;
    it->second.resolved_at = QDateTime::currentDateTime();
    stats_.alerts_resolved++;
    
    emit alert_resolved(alert_id);
    return true;
  }
  return false;
}

bool MonitoringManager::clear_all_alerts() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  alerts_.clear();
  return true;
}

// ── Metric-Definition ────────────────────────────────────────────────

bool MonitoringManager::register_metric(const MetricDefinition& metric) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  metrics_[metric.name] = metric;
  return true;
}

bool MonitoringManager::unregister_metric(const QString& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = metrics_.find(name);
  if (it != metrics_.end()) {
    metrics_.erase(it);
    return true;
  }
  return false;
}

std::vector<MonitoringManager::MetricDefinition> MonitoringManager::get_registered_metrics() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<MetricDefinition> result;
  for (const auto& pair : metrics_) {
    result.push_back(pair.second);
  }
  return result;
}

// ── Metric-Werte setzen ──────────────────────────────────────────────

bool MonitoringManager::set_gauge(const QString& name, double value, const std::map<QString, QString>& labels) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = metrics_.find(name);
  if (it == metrics_.end() || it->second.type != "gauge") {
    return false;
  }
  
  gauge_values_[name][labels] = value;
  stats_.metrics_sent++;
  
  emit metric_sent(name);
  return true;
}

bool MonitoringManager::increment_counter(const QString& name, double value, const std::map<QString, QString>& labels) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = metrics_.find(name);
  if (it == metrics_.end() || it->second.type != "counter") {
    return false;
  }
  
  counter_values_[name][labels] += value;
  stats_.metrics_sent++;
  
  emit metric_sent(name);
  return true;
}

bool MonitoringManager::observe_histogram(const QString& name, double value, const std::map<QString, QString>& labels) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = metrics_.find(name);
  if (it == metrics_.end() || it->second.type != "histogram") {
    return false;
  }
  
  histogram_values_[name].push_back(value);
  stats_.metrics_sent++;
  
  emit metric_sent(name);
  return true;
}

// ── Metric-Werte abrufen ─────────────────────────────────────────────

double MonitoringManager::get_gauge_value(const QString& name, const std::map<QString, QString>& labels) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = gauge_values_.find(name);
  if (it != gauge_values_.end()) {
    auto lit = it->second.find(labels);
    if (lit != it->second.end()) {
      return lit->second;
    }
  }
  return 0.0;
}

double MonitoringManager::get_counter_value(const QString& name, const std::map<QString, QString>& labels) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = counter_values_.find(name);
  if (it != counter_values_.end()) {
    auto lit = it->second.find(labels);
    if (lit != it->second.end()) {
      return lit->second;
    }
  }
  return 0.0;
}

// ── Dashboard-Management ─────────────────────────────────────────────

std::vector<MonitoringManager::Dashboard> MonitoringManager::get_dashboards() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dashboards_;
}

MonitoringManager::Dashboard* MonitoringManager::get_dashboard(const QString& uid) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& dashboard : dashboards_) {
    if (dashboard.uid == uid) {
      return &dashboard;
    }
  }
  return nullptr;
}

const MonitoringManager::Dashboard* MonitoringManager::get_dashboard(const QString& uid) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (const auto& dashboard : dashboards_) {
    if (dashboard.uid == uid) {
      return &dashboard;
    }
  }
  return nullptr;
}

bool MonitoringManager::add_dashboard(const Dashboard& dashboard) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  dashboards_.push_back(dashboard);
  return true;
}

bool MonitoringManager::remove_dashboard(const QString& uid) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto it = dashboards_.begin(); it != dashboards_.end(); ++it) {
    if (it->uid == uid) {
      dashboards_.erase(it);
      return true;
    }
  }
  return false;
}

// ── Health-Check ─────────────────────────────────────────────────────

MonitoringManager::HealthStatus MonitoringManager::get_health_status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return health_status_;
}

// ── Alerting-Regeln ──────────────────────────────────────────────────

bool MonitoringManager::add_alert_rule(const AlertRule& rule) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  alert_rules_.push_back(rule);
  return true;
}

bool MonitoringManager::remove_alert_rule(const QString& rule_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto it = alert_rules_.begin(); it != alert_rules_.end(); ++it) {
    if (it->id == rule_id) {
      alert_rules_.erase(it);
      return true;
    }
  }
  return false;
}

std::vector<MonitoringManager::AlertRule> MonitoringManager::get_alert_rules() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return alert_rules_;
}

bool MonitoringManager::enable_alert_rule(const QString& rule_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& rule : alert_rules_) {
    if (rule.id == rule_id) {
      rule.is_enabled = true;
      return true;
    }
  }
  return false;
}

bool MonitoringManager::disable_alert_rule(const QString& rule_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& rule : alert_rules_) {
    if (rule.id == rule_id) {
      rule.is_enabled = false;
      return true;
    }
  }
  return false;
}

// ── Statistik ────────────────────────────────────────────────────────

MonitoringManager::Stats MonitoringManager::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void MonitoringManager::reset_stats() {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_ = Stats{};
}

// ── Callbacks ────────────────────────────────────────────────────────

void MonitoringManager::set_alert_callback(AlertCallback callback) {
  alert_callback_ = std::move(callback);
}

void MonitoringManager::set_health_callback(HealthCallback callback) {
  health_callback_ = std::move(callback);
}

void MonitoringManager::set_log_callback(LogCallback callback) {
  log_callback_ = std::move(callback);
}

void MonitoringManager::set_audit_callback(AuditCallback callback) {
  audit_callback_ = std::move(callback);
}

// ── Log-Aggregation ──────────────────────────────────────────────────

std::vector<MonitoringManager::LogEntry> MonitoringManager::get_logs(LogLevel level) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<LogEntry> result;
  for (const auto& log : logs_) {
    if (log.level == level) {
      result.push_back(log);
    }
  }
  return result;
}

std::vector<MonitoringManager::LogEntry> MonitoringManager::get_logs_by_source(const QString& source) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<LogEntry> result;
  for (const auto& log : logs_) {
    if (log.source == source) {
      result.push_back(log);
    }
  }
  return result;
}

std::vector<MonitoringManager::LogEntry> MonitoringManager::get_recent_logs(int count) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<LogEntry> result;
  int start = static_cast<int>(logs_.size()) - count;
  if (start < 0) {
    start = 0;
  }
  for (int i = start; i < static_cast<int>(logs_.size()); i++) {
    result.push_back(logs_[i]);
  }
  return result;
}

bool MonitoringManager::add_log(const LogEntry& entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  logs_.push_back(entry);
  
  if (log_callback_) {
    log_callback_(entry);
  }
  
  return true;
}

bool MonitoringManager::clear_logs() {
  std::lock_guard<std::mutex> lock(mutex_);
  logs_.clear();
  return true;
}

size_t MonitoringManager::get_log_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return logs_.size();
}

// ── Audit-Log ────────────────────────────────────────────────────────

std::vector<MonitoringManager::AuditEntry> MonitoringManager::get_audit_logs() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return audit_logs_;
}

std::vector<MonitoringManager::AuditEntry> MonitoringManager::get_audit_logs_by_user(const QString& user_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<AuditEntry> result;
  for (const auto& entry : audit_logs_) {
    if (entry.user_id == user_id) {
      result.push_back(entry);
    }
  }
  return result;
}

std::vector<MonitoringManager::AuditEntry> MonitoringManager::get_audit_logs_by_action(const QString& action) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<AuditEntry> result;
  for (const auto& entry : audit_logs_) {
    if (entry.action == action) {
      result.push_back(entry);
    }
  }
  return result;
}

bool MonitoringManager::add_audit_entry(const AuditEntry& entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  audit_logs_.push_back(entry);
  
  if (audit_callback_) {
    audit_callback_(entry);
  }
  
  return true;
}

bool MonitoringManager::clear_audit_logs() {
  std::lock_guard<std::mutex> lock(mutex_);
  audit_logs_.clear();
  return true;
}

size_t MonitoringManager::get_audit_log_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return audit_logs_.size();
}

// ── Slots ────────────────────────────────────────────────────────────

void MonitoringManager::on_health_check_timer() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Simuliere Health-Check
  health_status_.is_healthy = true;
  health_status_.status_message = "OK";
  health_status_.uptime_ms += 1000;
  health_status_.cpu_usage_percent = static_cast<double>(QRandomGenerator::system()->bounded(100));
  health_status_.memory_usage_percent = static_cast<double>(QRandomGenerator::system()->bounded(100));
  health_status_.network_latency_ms = static_cast<double>(QRandomGenerator::system()->bounded(100));
  health_status_.last_check = QDateTime::currentDateTime();
  
  stats_.health_checks++;
  
  emit health_check_completed(health_status_);
  
  if (health_callback_) {
    health_callback_(health_status_);
  }
}

void MonitoringManager::on_metric_send_timer() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Simuliere Metric-Send
  stats_.last_metric_send_timestamp = QDateTime::currentMSecsSinceEpoch();
}

} // namespace hh::client

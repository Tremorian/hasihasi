// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cluster_health.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <chrono>

namespace hh::server {

// ── ClusterHealth ─────────────────────────────────────────────────────

ClusterHealth::ClusterHealth(ClusterHealth&& other) noexcept
  : cluster_health_status_(other.cluster_health_status_)
  , reporting_enabled_(other.reporting_enabled_)
  , reporting_interval_ms_(other.reporting_interval_ms_)
  , metrics_(std::move(other.metrics_))
  , health_status_changed_callback_(std::move(other.health_status_changed_callback_))
  , metric_changed_callback_(std::move(other.metric_changed_callback_))
  , total_checks_(other.total_checks_.load())
  , successful_checks_(other.successful_checks_.load())
  , failed_checks_(other.failed_checks_.load()) {
  
  other.cluster_health_status_ = HealthStatus::Healthy;
  other.reporting_enabled_ = false;
  other.reporting_interval_ms_ = 30000;
  other.total_checks_ = 0;
  other.successful_checks_ = 0;
  other.failed_checks_ = 0;
}

ClusterHealth& ClusterHealth::operator=(ClusterHealth&& other) noexcept {
  if (this != &other) {
    cluster_health_status_ = other.cluster_health_status_;
    reporting_enabled_ = other.reporting_enabled_;
    reporting_interval_ms_ = other.reporting_interval_ms_;
    metrics_ = std::move(other.metrics_);
    health_status_changed_callback_ = std::move(other.health_status_changed_callback_);
    metric_changed_callback_ = std::move(other.metric_changed_callback_);
    total_checks_ = other.total_checks_.load();
    successful_checks_ = other.successful_checks_.load();
    failed_checks_ = other.failed_checks_.load();
    other.cluster_health_status_ = HealthStatus::Healthy;
    other.reporting_enabled_ = false;
    other.reporting_interval_ms_ = 30000;
    other.total_checks_ = 0;
    other.successful_checks_ = 0;
    other.failed_checks_ = 0;
  }
  return *this;
}

ClusterHealth::HealthStatus ClusterHealth::check_cluster_health(ServerCluster& cluster) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto start = std::chrono::steady_clock::now();
  
  total_checks_++;
  
  auto servers = cluster.get_all_servers();
  if (servers.empty()) {
    cluster_health_status_ = HealthStatus::Critical;
    if (health_status_changed_callback_) {
      health_status_changed_callback_(cluster_health_status_);
    }
    return cluster_health_status_;
  }
  
  uint32_t healthy_count = 0;
  for (const auto& server : servers) {
    if (check_server_health_internal(server)) {
      healthy_count++;
    }
  }
  
  double healthy_ratio = static_cast<double>(healthy_count) / servers.size();
  
  if (healthy_ratio >= 0.8) {
    cluster_health_status_ = HealthStatus::Healthy;
  } else if (healthy_ratio >= 0.5) {
    cluster_health_status_ = HealthStatus::Degraded;
  } else {
    cluster_health_status_ = HealthStatus::Unhealthy;
  }
  
  auto end = std::chrono::steady_clock::now();
  double duration = std::chrono::duration<double, std::milli>(end - start).count();
  
  successful_checks_++;
  
  if (health_status_changed_callback_) {
    health_status_changed_callback_(cluster_health_status_);
  }
  
  hh::get_logger().info("ClusterHealth: Cluster status: {}, {}/{} servers healthy",
                        cluster_health_status_ == HealthStatus::Healthy ? "Healthy" :
                        cluster_health_status_ == HealthStatus::Degraded ? "Degraded" :
                        cluster_health_status_ == HealthStatus::Unhealthy ? "Unhealthy" : "Critical",
                        healthy_count, servers.size());
  
  return cluster_health_status_;
}

ClusterHealth::HealthStatus ClusterHealth::check_server_health(const ServerCluster::ServerInfo& server) {
  return check_server_health_internal(server) ? HealthStatus::Healthy : HealthStatus::Unhealthy;
}

void ClusterHealth::add_metric(const HealthMetric& metric) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  metrics_[metric.name] = metric;
  
  if (metric_changed_callback_) {
    metric_changed_callback_(metric);
  }
}

void ClusterHealth::remove_metric(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  metrics_.erase(name);
}

ClusterHealth::HealthMetric* ClusterHealth::get_metric(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = metrics_.find(name);
  if (it != metrics_.end()) {
    return &it->second;
  }
  return nullptr;
}

const ClusterHealth::HealthMetric* ClusterHealth::get_metric(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = metrics_.find(name);
  if (it != metrics_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<ClusterHealth::HealthMetric> ClusterHealth::get_all_metrics() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<HealthMetric> metrics;
  for (const auto& [name, metric] : metrics_) {
    metrics.push_back(metric);
  }
  return metrics;
}

ClusterHealth::HealthStatus ClusterHealth::get_cluster_health_status() const {
  return cluster_health_status_;
}

bool ClusterHealth::is_cluster_healthy() const {
  return cluster_health_status_ == HealthStatus::Healthy;
}

bool ClusterHealth::is_cluster_degraded() const {
  return cluster_health_status_ == HealthStatus::Degraded;
}

bool ClusterHealth::is_cluster_unhealthy() const {
  return cluster_health_status_ == HealthStatus::Unhealthy || cluster_health_status_ == HealthStatus::Critical;
}

void ClusterHealth::set_health_status_changed_callback(HealthStatusChangedCallback callback) {
  health_status_changed_callback_ = std::move(callback);
}

void ClusterHealth::set_metric_changed_callback(MetricChangedCallback callback) {
  metric_changed_callback_ = std::move(callback);
}

void ClusterHealth::set_reporting_enabled(bool enabled) {
  reporting_enabled_ = enabled;
}

void ClusterHealth::set_reporting_interval_ms(int interval_ms) {
  reporting_interval_ms_ = interval_ms;
}

ClusterHealth::HealthStats ClusterHealth::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  HealthStats stats;
  stats.cluster_status = cluster_health_status_;
  stats.total_metrics = metrics_.size();
  stats.unhealthy_metrics = std::count_if(metrics_.begin(), metrics_.end(),
                                          [](const auto& pair) { return !pair.second.is_healthy; });
  stats.last_check_duration_ms = 0.0; // Wird beim Check aktualisiert
  
  return stats;
}

void ClusterHealth::reset_stats() {
  total_checks_ = 0;
  successful_checks_ = 0;
  failed_checks_ = 0;
}

void ClusterHealth::check_health_internal() {
  // Health-Check durchführen
  // In einer echten Implementierung würde hier der Check mit dem Cluster erfolgen
}

bool ClusterHealth::check_server_health_internal(const ServerCluster::ServerInfo& server) {
  return server.is_healthy;
}

bool ClusterHealth::check_metric_health(const HealthMetric& metric) {
  if (metric.value > metric.threshold_critical) {
    return false;
  }
  return true;
}

} // namespace hh::server

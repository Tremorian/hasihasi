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

#include "server_cluster.hpp"

namespace hh::server {

// Cluster-Health: Monitoring für Server-Cluster
class ClusterHealth {
public:
  enum class HealthStatus {
    Healthy,
    Degraded,
    Unhealthy,
    Critical,
    Count
  };

  struct HealthMetric {
    std::string name;
    double value = 0.0;
    double threshold_warning = 0.0;
    double threshold_critical = 0.0;
    bool is_healthy = true;
  };

  ClusterHealth() = default;
  ~ClusterHealth() = default;

  // Nicht kopierbar, bewegbar
  ClusterHealth(const ClusterHealth&) = delete;
  ClusterHealth& operator=(const ClusterHealth&) = delete;
  ClusterHealth(ClusterHealth&&) noexcept;
  ClusterHealth& operator=(ClusterHealth&&) noexcept;

  // Health prüfen
  HealthStatus check_cluster_health(ServerCluster& cluster);
  HealthStatus check_server_health(const ServerCluster::ServerInfo& server);

  // Health-Metriken
  void add_metric(const HealthMetric& metric);
  void remove_metric(const std::string& name);
  HealthMetric* get_metric(const std::string& name);
  const HealthMetric* get_metric(const std::string& name) const;
  std::vector<HealthMetric> get_all_metrics() const;

  // Health-Status
  HealthStatus get_cluster_health_status() const;
  bool is_cluster_healthy() const;
  bool is_cluster_degraded() const;
  bool is_cluster_unhealthy() const;

  // Health-Callbacks
  using HealthStatusChangedCallback = std::function<void(HealthStatus status)>;
  using MetricChangedCallback = std::function<void(const HealthMetric& metric)>;
  
  void set_health_status_changed_callback(HealthStatusChangedCallback callback);
  void set_metric_changed_callback(MetricChangedCallback callback);

  // Health-Reporting
  void set_reporting_enabled(bool enabled);
  bool is_reporting_enabled() const { return reporting_enabled_; }

  void set_reporting_interval_ms(int interval_ms);
  int get_reporting_interval_ms() const { return reporting_interval_ms_; }

  // Statistiken
  struct HealthStats {
    HealthStatus cluster_status = HealthStatus::Healthy;
    uint32_t total_servers = 0;
    uint32_t healthy_servers = 0;
    uint32_t unhealthy_servers = 0;
    uint32_t total_metrics = 0;
    uint32_t unhealthy_metrics = 0;
    double last_check_duration_ms = 0.0;
  };

  HealthStats get_stats() const;
  void reset_stats();

private:
  // Health prüfen
  void check_health_internal();
  
  // Server-Health prüfen
  bool check_server_health_internal(const ServerCluster::ServerInfo& server);
  
  // Metric-Health prüfen
  bool check_metric_health(const HealthMetric& metric);
  
  // Einstellungen
  HealthStatus cluster_health_status_ = HealthStatus::Healthy;
  bool reporting_enabled_ = false;
  int reporting_interval_ms_ = 30000; // 30 Sekunden
  
  // Health-Metriken
  mutable std::mutex mutex_;
  std::unordered_map<std::string, HealthMetric> metrics_;
  
  // Callbacks
  HealthStatusChangedCallback health_status_changed_callback_;
  MetricChangedCallback metric_changed_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> total_checks_{0};
  mutable std::atomic<uint32_t> successful_checks_{0};
  mutable std::atomic<uint32_t> failed_checks_{0};
};

} // namespace hh::server

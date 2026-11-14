// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace hh::client {

// Monitoring & Alerting: Prometheus-Erweiterung, Grafana-Dashboards, Alerting
class MonitoringManager : public QObject {
  Q_OBJECT

public:
  explicit MonitoringManager(QObject* parent = nullptr);
  ~MonitoringManager() override = default;

  // Nicht kopierbar, bewegbar
  MonitoringManager(const MonitoringManager&) = delete;
  MonitoringManager& operator=(const MonitoringManager&) = delete;
  MonitoringManager(MonitoringManager&&) noexcept;
  MonitoringManager& operator=(MonitoringManager&&) noexcept;

  // Prometheus
  enum class PrometheusStatus {
    DISABLED,
    CONNECTED,
    ERROR
  };
  Q_ENUM(PrometheusStatus)

  PrometheusStatus prometheus_status() const;
  bool set_prometheus_url(const QString& url);
  QString prometheus_url() const;

  // Grafana
  enum class GrafanaStatus {
    DISABLED,
    CONNECTED,
    ERROR
  };
  Q_ENUM(GrafanaStatus)

  GrafanaStatus grafana_status() const;
  bool set_grafana_url(const QString& url);
  bool set_grafana_api_key(const QString& key);
  QString grafana_url() const;
  QString grafana_api_key() const;

  // Alerting
  enum class AlertSeverity {
    INFO,
    WARNING,
    CRITICAL,
    EMERGENCY
  };
  Q_ENUM(AlertSeverity)

  struct Alert {
    QString id;
    QString message;
    AlertSeverity severity;
    QDateTime created_at;
    QDateTime resolved_at;
    bool is_resolved;
    QString source;
    QString metric_name;
    double metric_value;
    double threshold;
  };

  std::vector<Alert> get_alerts(AlertSeverity severity = AlertSeverity::INFO) const;
  std::vector<Alert> get_active_alerts() const;
  bool resolve_alert(const QString& alert_id);
  bool clear_all_alerts();

  // Metric-Definition
  struct MetricDefinition {
    QString name;
    QString type; // gauge, counter, histogram
    QString help;
    std::vector<QString> labels;
  };

  bool register_metric(const MetricDefinition& metric);
  bool unregister_metric(const QString& name);
  std::vector<MetricDefinition> get_registered_metrics() const;

  // Metric-Werte setzen
  bool set_gauge(const QString& name, double value, const std::map<QString, QString>& labels = {});
  bool increment_counter(const QString& name, double value = 1.0, const std::map<QString, QString>& labels = {});
  bool observe_histogram(const QString& name, double value, const std::map<QString, QString>& labels = {});

  // Metric-Werte abrufen
  double get_gauge_value(const QString& name, const std::map<QString, QString>& labels = {}) const;
  double get_counter_value(const QString& name, const std::map<QString, QString>& labels = {}) const;

  // Dashboard-Management
  struct Dashboard {
    QString id;
    QString title;
    QString uid;
    QString url;
    std::vector<QString> tags;
    QDateTime created_at;
    QDateTime updated_at;
  };

  std::vector<Dashboard> get_dashboards() const;
  Dashboard* get_dashboard(const QString& uid);
  const Dashboard* get_dashboard(const QString& uid) const;
  bool add_dashboard(const Dashboard& dashboard);
  bool remove_dashboard(const QString& uid);

  // Health-Check
  struct HealthStatus {
    bool is_healthy;
    QString status_message;
    qint64 uptime_ms;
    int connected_users;
    int active_channels;
    double cpu_usage_percent;
    double memory_usage_percent;
    double network_latency_ms;
    QDateTime last_check;
  };

  HealthStatus get_health_status() const;

  // Alerting-Regeln
  struct AlertRule {
    QString id;
    QString name;
    QString metric_name;
    double threshold;
    AlertSeverity severity;
    QString message;
    bool is_enabled;
    qint64 evaluation_interval_ms; // Millisekunden
  };

  bool add_alert_rule(const AlertRule& rule);
  bool remove_alert_rule(const QString& rule_id);
  std::vector<AlertRule> get_alert_rules() const;
  bool enable_alert_rule(const QString& rule_id);
  bool disable_alert_rule(const QString& rule_id);

  // Statistik
  struct Stats {
    size_t metrics_sent;
    size_t alerts_triggered;
    size_t alerts_resolved;
    size_t health_checks;
    qint64 last_metric_send_timestamp;
    qint64 last_health_check_timestamp;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using AlertCallback = std::function<void(const Alert& alert)>;
  using HealthCallback = std::function<void(const HealthStatus& status)>;

  void set_alert_callback(AlertCallback callback);
  void set_health_callback(HealthCallback callback);

signals:
  void prometheus_connected();
  void prometheus_disconnected();
  void grafana_connected();
  void grafana_disconnected();
  void alert_triggered(const Alert& alert);
  void alert_resolved(const QString& alert_id);
  void health_check_completed(const HealthStatus& status);
  void metric_sent(const QString& name);

private slots:
  void on_health_check_timer();
  void on_metric_send_timer();

private:
  // Prometheus
  PrometheusStatus prometheus_status_ = PrometheusStatus::DISABLED;
  QString prometheus_url_;
  
  // Grafana
  GrafanaStatus grafana_status_ = GrafanaStatus::DISABLED;
  QString grafana_url_;
  QString grafana_api_key_;
  
  // Alerts
  std::map<QString, Alert> alerts_;
  QString current_alert_id_;
  
  // Metrics
  std::map<QString, MetricDefinition> metrics_;
  std::map<QString, std::map<std::map<QString, QString>, double>> gauge_values_;
  std::map<QString, std::map<std::map<QString, QString>, double>> counter_values_;
  std::map<QString, std::vector<double>> histogram_values_;
  
  // Dashboards
  std::vector<Dashboard> dashboards_;
  
  // Health-Check
  HealthStatus health_status_;
  
  // Alerting-Regeln
  std::vector<AlertRule> alert_rules_;
  
  // Statistik
  Stats stats_;
  
  AlertCallback alert_callback_;
  HealthCallback health_callback_;
  
  mutable std::mutex mutex_;
};

} // namespace hh::client

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <mutex>
#include <atomic>

namespace hh::server {

// Prometheus-Metriken für den HasiHasi-Server
class PrometheusMetrics {
public:
  PrometheusMetrics() = default;
  ~PrometheusMetrics() = default;

  // Initialisierung
  bool initialize(uint16_t port = 9191, const std::string& bind_address = "127.0.0.1");
  void shutdown();
  bool is_running() const { return is_running_; }

  // Metrik-Typen
  enum class Type {
    COUNTER,      // Kumulative Werte (z.B. packets_sent)
    GAUGE,        // Aktuelle Werte (z.B. active_users)
    HISTOGRAM,    // Verteilungen (z.B. latency_ms)
    SUMMARY      // Quantile (z.B. latency_percentiles)
  };

  // Metrik registrieren
  struct MetricInfo {
    std::string name;
    std::string help;
    Type type;
    std::map<std::string, std::string> labels;
  };

  bool register_counter(const std::string& name, const std::string& help,
                        const std::map<std::string, std::string>& labels = {});
  bool register_gauge(const std::string& name, const std::string& help,
                      const std::map<std::string, std::string>& labels = {});
  bool register_histogram(const std::string& name, const std::string& help,
                          const std::map<std::string, std::string>& labels = {});

  // Metriken aktualisieren
  void increment_counter(const std::string& name, const std::map<std::string, std::string>& labels = {});
  void set_gauge(const std::string& name, double value, const std::map<std::string, std::string>& labels = {});
  void observe_histogram(const std::string& name, double value, const std::map<std::string, std::string>& labels = {});

  // HasiHasi-spezifische Metriken
  void init_default_metrics();

  // HTTP-Endpoint für /metrics
  std::string scrape() const;

  // Konfiguration
  struct Config {
    uint16_t port = 9191;
    std::string bind_address = "127.0.0.1";
    bool enabled = true;
  };

  void set_config(const Config& config);
  Config config() const { return config_; }

  // Callbacks
  using MetricsCallback = std::function<void(const std::string& metrics)>;
  void set_metrics_callback(MetricsCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  Config config_;
  bool is_running_ = false;
  MetricsCallback metrics_callback_;

  // Metrik-Speicherung
  struct MetricValue {
    double value = 0.0;
    std::map<std::string, std::string> labels;
  };

  std::map<std::string, MetricValue> counters_;
  std::map<std::string, MetricValue> gauges_;
  std::map<std::string, std::vector<double>> histograms_;
};

} // namespace hh::server

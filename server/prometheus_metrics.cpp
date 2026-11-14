// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/prometheus_metrics.hpp"
#include "hh/logging.hpp"

#include <sstream>
#include <iomanip>
#include <chrono>

namespace hh::server {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct PrometheusMetrics::Impl {
  asio::io_context& ioc_;
  asio::ip::tcp::acceptor acceptor_;
  bool running_ = false;

  Impl(asio::io_context& ioc)
    : ioc_(ioc)
    , acceptor_(ioc, asio::ip::tcp::v4()) {
  }

  ~Impl() = default;

  bool start(uint16_t port, const std::string& bind_address) {
    std::error_code ec;
    asio::ip::tcp::endpoint endpoint(
      asio::ip::make_address(bind_address), port);
    
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      hh::get_logger().error("PrometheusMetrics: Failed to open socket: {}", ec.message());
      return false;
    }

    acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
      hh::get_logger().warn("PrometheusMetrics: Failed to set reuse_address: {}", ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
      hh::get_logger().error("PrometheusMetrics: Failed to bind to {}:{}", 
                             bind_address, port, ec.message());
      return false;
    }

    acceptor_.listen(1, ec);
    if (ec) {
      hh::get_logger().error("PrometheusMetrics: Failed to listen: {}", ec.message());
      return false;
    }

    running_ = true;
    hh::get_logger().info("PrometheusMetrics: HTTP server started on {}:{}", 
                          bind_address, port);

    // Accept-Loop starten
    do_accept();
    return true;
  }

  void stop() {
    running_ = false;
    std::error_code ec;
    acceptor_.close(ec);
    hh::get_logger().info("PrometheusMetrics: HTTP server stopped");
  }

  void do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
      if (!ec) {
        handle_request(std::move(socket));
      }
      if (running_) {
        do_accept();
      }
    });
  }

  void handle_request(asio::ip::tcp::socket socket) {
    std::array<char, 4096> buffer;
    std::error_code ec;
    size_t bytes_transferred = socket.receive(asio::buffer(buffer), 0, ec);

    if (ec) {
      return;
    }

    // Einfache HTTP-Anfrage parsen
    std::string request(buffer.data(), bytes_transferred);
    
    // Prüfe auf GET /metrics
    bool is_metrics_request = (request.find("GET /metrics") != std::string::npos);

    // Antwort aufbauen
    std::stringstream response;
    if (is_metrics_request) {
      response << "HTTP/1.1 200 OK\r\n";
      response << "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n";
      response << "Content-Length: 0\r\n";
      response << "\r\n";
    } else {
      response << "HTTP/1.1 404 Not Found\r\n";
      response << "Content-Type: text/plain\r\n";
      response << "Content-Length: 9\r\n";
      response << "\r\n";
      response << "Not Found";
    }

    // Antwort senden
    asio::write(socket, asio::buffer(response.str()), ec);
  }
};

// ── PrometheusMetrics ─────────────────────────────────────────────────

bool PrometheusMetrics::initialize(uint16_t port, const std::string& bind_address) {
  if (is_running_) {
    return true;
  }

  // IO-Context für HTTP-Server erstellen
  auto ioc = std::make_shared<asio::io_context>();
  auto ioc_strand = std::make_shared<asio::io_context::strand>(*ioc);
  
  impl_ = std::make_unique<Impl>(*ioc);
  
  if (!impl_->start(port, bind_address)) {
    return false;
  }

  is_running_ = true;
  return true;
}

void PrometheusMetrics::shutdown() {
  if (!is_running_) {
    return;
  }

  if (impl_) {
    impl_->stop();
  }
  is_running_ = false;
}

bool PrometheusMetrics::register_counter(const std::string& name, const std::string& help,
                                          const std::map<std::string, std::string>& labels) {
  counters_[name] = MetricValue{0.0, labels};
  hh::get_logger().debug("PrometheusMetrics: Registered counter {}", name);
  return true;
}

bool PrometheusMetrics::register_gauge(const std::string& name, const std::string& help,
                                        const std::map<std::string, std::string>& labels) {
  gauges_[name] = MetricValue{0.0, labels};
  hh::get_logger().debug("PrometheusMetrics: Registered gauge {}", name);
  return true;
}

bool PrometheusMetrics::register_histogram(const std::string& name, const std::string& help,
                                            const std::map<std::string, std::string>& labels) {
  histograms_[name] = {};
  hh::get_logger().debug("PrometheusMetrics: Registered histogram {}", name);
  return true;
}

void PrometheusMetrics::increment_counter(const std::string& name,
                                           const std::map<std::string, std::string>& labels) {
  auto it = counters_.find(name);
  if (it != counters_.end()) {
    it->second.value += 1.0;
  }
}

void PrometheusMetrics::set_gauge(const std::string& name, double value,
                                   const std::map<std::string, std::string>& labels) {
  auto it = gauges_.find(name);
  if (it != gauges_.end()) {
    it->second.value = value;
  }
}

void PrometheusMetrics::observe_histogram(const std::string& name, double value,
                                           const std::map<std::string, std::string>& labels) {
  auto it = histograms_.find(name);
  if (it != histograms_.end()) {
    it->second.push_back(value);
  }
}

void PrometheusMetrics::init_default_metrics() {
  // Server-Statistiken
  register_gauge("hasihi_active_users", "Anzahl aktiver Nutzer");
  register_gauge("hasihi_active_channels", "Anzahl aktiver Kanäle");
  register_gauge("hasihi_active_speakers", "Anzahl aktiver Sprecher");
  
  // Netzwerk-Statistiken
  register_counter("hasihi_packets_sent_total", "Gesendete Voice-Pakete");
  register_counter("hasihi_packets_received_total", "Empfangene Voice-Pakete");
  register_counter("hasihi_bytes_sent_total", "Gesendete Bytes");
  register_counter("hasihi_bytes_received_total", "Empfangene Bytes");
  register_counter("hasihi_key_rotations_total", "Schlüssel-Rotationen");
  
  // Auth-Statistiken
  register_counter("hasihi_auth_success_total", "Erfolgreiche Authentifizierungen");
  register_counter("hasihi_auth_failure_total", "Fehlgeschlagene Authentifizierungen");
  register_counter("hasihi_tofu_accepts_total", "TOFU-Akzeptierungen");
  register_counter("hasihi_tofu_rejects_total", "TOFU-Ablehnungen");
  
  // Latenz-Histogramm
  register_histogram("hasihi_latency_ms", "Mund-zu-Ohr-Latenz in ms");
  
  hh::get_logger().info("PrometheusMetrics: Default metrics initialized");
}

std::string PrometheusMetrics::scrape() const {
  std::stringstream ss;

  // Counter
  for (const auto& [name, value] : counters_) {
    ss << "# HELP " << name << " HasiHasi counter metric\n";
    ss << "# TYPE " << name << " counter\n";
    ss << name << " " << std::fixed << std::setprecision(0) << value.value << "\n";
  }

  // Gauge
  for (const auto& [name, value] : gauges_) {
    ss << "# HELP " << name << " HasiHasi gauge metric\n";
    ss << "# TYPE " << name << " gauge\n";
    ss << name << " " << std::fixed << std::setprecision(2) << value.value << "\n";
  }

  // Histogramm
  for (const auto& [name, values] : histograms_) {
    ss << "# HELP " << name << " HasiHasi histogram metric\n";
    ss << "# TYPE " << name << " histogram\n";
    if (!values.empty()) {
      double sum = 0.0;
      for (double v : values) {
        sum += v;
      }
      double avg = sum / values.size();
      ss << name << "_sum " << std::fixed << std::setprecision(2) << sum << "\n";
      ss << name << "_count " << values.size() << "\n";
      ss << name << "_avg " << std::fixed << std::setprecision(2) << avg << "\n";
    }
  }

  return ss.str();
}

void PrometheusMetrics::set_config(const Config& config) {
  config_ = config;
}

PrometheusMetrics::Config PrometheusMetrics::config() const {
  return config_;
}

void PrometheusMetrics::set_metrics_callback(MetricsCallback callback) {
  metrics_callback_ = callback;
}

} // namespace hh::server

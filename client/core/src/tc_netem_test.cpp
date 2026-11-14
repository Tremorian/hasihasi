// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/tc_netem_test.hpp"

#include "hh/logging.hpp"

#include <sstream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>

namespace hh::client {

// ── TcNetemTest ───────────────────────────────────────────────────────

TcNetemTest::TcNetemTest(TcNetemTest&& other) noexcept
  : active_(other.active_)
  , current_config_(other.current_config_) {
  
  other.active_ = false;
}

TcNetemTest& TcNetemTest::operator=(TcNetemTest&& other) noexcept {
  if (this != &other) {
    reset();
    
    active_ = other.active_;
    current_config_ = other.current_config_;
    
    other.active_ = false;
  }
  return *this;
}

bool TcNetemTest::check_root() {
  return geteuid() == 0;
}

bool TcNetemTest::apply(const Config& config) {
  if (!check_root()) {
    hh::get_logger().error("tc netem: Requires root privileges");
    return false;
  }
  
  // Bestehende Regeln entfernen
  execute_command("sudo tc qdisc del dev " + config.interface + " root 2>/dev/null");
  
  // tc netem-Regeln anwenden
  std::string cmd = "sudo tc qdisc add dev " + config.interface + " root handle 1: netem";
  
  if (config.delay_ms > 0.0) {
    cmd += " delay " + std::to_string(config.delay_ms) + "ms";
    if (config.jitter_ms > 0.0) {
      cmd += " " + std::to_string(config.jitter_ms) + "ms";
    }
  }
  
  if (config.loss_percentage > 0.0) {
    cmd += " loss " + std::to_string(config.loss_percentage) + "%";
  }
  
  if (config.duplicate_percentage > 0.0) {
    cmd += " duplicate " + std::to_string(config.duplicate_percentage) + "%";
  }
  
  if (config.corruption_percentage > 0.0) {
    cmd += " corruption " + std::to_string(config.corruption_percentage) + "%";
  }
  
  if (config.reorder_percentage > 0.0) {
    cmd += " reorder " + std::to_string(config.reorder_percentage) + "%";
  }
  
  if (config.fragmentation) {
    cmd += " corrupt";
  }
  
  if (config.rate_bps > 0.0) {
    cmd += " rate " + std::to_string(config.rate_bps) + "bit";
  }
  
  std::string output = execute_command(cmd);
  if (!output.empty()) {
    hh::get_logger().error("tc netem: Failed to apply config: {}", output);
    return false;
  }
  
  current_config_ = config;
  active_ = true;
  
  hh::get_logger().info("tc netem: Applied config on {} (delay: {}ms, jitter: {}ms, loss: {}%)",
                        config.interface, config.delay_ms, config.jitter_ms, config.loss_percentage);
  return true;
}

bool TcNetemTest::reset() {
  if (!active_) {
    return true;
  }
  
  std::string cmd = "sudo tc qdisc del dev " + current_config_.interface + " root 2>/dev/null";
  std::string output = execute_command(cmd);
  
  if (!output.empty()) {
    hh::get_logger().error("tc netem: Failed to reset config: {}", output);
    return false;
  }
  
  active_ = false;
  current_config_ = Config{};
  
  hh::get_logger().info("tc netem: Reset config on {}", current_config_.interface);
  return true;
}

std::string TcNetemTest::get_status() const {
  if (!active_) {
    return "tc netem is not active";
  }
  
  std::string cmd = "sudo tc -s qdisc show dev " + current_config_.interface;
  std::string output = execute_command(cmd);
  
  return output;
}

TcNetemTest::TestResult TcNetemTest::run_test(const Config& config, int num_packets) {
  TestResult result;
  result.packets_sent = num_packets;
  
  // tc netem anwenden
  if (!apply(config)) {
    hh::get_logger().error("tc netem: Failed to apply config for test");
    return result;
  }
  
  // Ping-Test ausführen
  std::string cmd = "ping -c " + std::to_string(num_packets) + 
                    " -i 0.01 " + config.interface;
  
  auto start_time = std::chrono::steady_clock::now();
  
  std::string output = execute_command(cmd);
  
  auto end_time = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
  
  // Ergebnisse parsen
  // ping: 100 packets transmitted, 95 received, 5% packet loss, time 1000ms
  // rtt min/avg/max/mdev = 1.0/5.0/10.0/2.0 ms
  std::istringstream iss(output);
  std::string line;
  
  while (std::getline(iss, line)) {
    if (line.find("packets transmitted") != std::string::npos) {
      std::istringstream line_stream(line);
      std::string dummy;
      line_stream >> dummy >> dummy >> dummy >> dummy >> result.packets_sent;
      line_stream >> dummy >> dummy >> dummy >> dummy >> result.packets_received;
      line_stream >> dummy >> dummy >> result.loss_percentage;
    } else if (line.find("rtt min/avg/max/mdev") != std::string::npos) {
      std::istringstream line_stream(line);
      std::string dummy;
      line_stream >> dummy >> dummy >> dummy;
      line_stream >> result.min_latency_ms;
      line_stream >> dummy >> result.avg_latency_ms;
      line_stream >> dummy >> result.max_latency_ms;
      line_stream >> dummy >> result.jitter_ms;
    }
  }
  
  result.packets_dropped = result.packets_sent - result.packets_received;
  
  // tc netem zurücksetzen
  reset();
  
  hh::get_logger().info("tc netem: Test completed (avg: {}ms, min: {}ms, max: {}ms, jitter: {}ms, loss: {}%)",
                        result.avg_latency_ms, result.min_latency_ms, result.max_latency_ms,
                        result.jitter_ms, result.loss_percentage);
  
  return result;
}

std::string TcNetemTest::execute_command(const std::string& command) const {
  std::string output;
  std::array<char, 256> buffer;
  
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
  if (!pipe) {
    return "";
  }
  
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    output += buffer.data();
  }
  
  return output;
}

TcNetemTest::Config TcNetemTest::parse_tc_output(const std::string& output) const {
  Config config;
  
  // Hier könnte man die tc-Ausgabe parsen
  // Für jetzt: Default-Werte zurückgeben
  return config;
}

} // namespace hh::client

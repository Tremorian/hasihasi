// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
<function_calls>
<invoke>
<function name="bash">
</invoke>
</function_calls>

// tc netem-Test für Netzwerk-Simulation
// Simuliert Paketverlust, Latenz und Jitter auf der Netzwerk-Interface-Ebene
class TcNetemTest {
public:
  TcNetemTest() = default;
  ~TcNetemTest() = default;

  // Nicht kopierbar, bewegbar
  TcNetemTest(const TcNetemTest&) = delete;
  TcNetemTest& operator=(const TcNetemTest&) = delete;
  TcNetemTest(TcNetemTest&&) noexcept;
  TcNetemTest& operator=(TcNetemTest&&) noexcept;

  // tc netem-Konfiguration
  struct Config {
    std::string interface = "lo"; // Loopback-Interface für Tests
    double rate_bps = 0.0; // Bandbreite in bps (0 = keine Begrenzung)
    double delay_ms = 0.0; // Basis-Latenz in ms
    double jitter_ms = 0.0; // Jitter in ms (abweichung von delay_ms)
    double loss_percentage = 0.0; // Paketverlust in %
    double duplicate_percentage = 0.0; // Duplikate in %
    double corruption_percentage = 0.0; // Korruption in %
    bool reorder_percentage = 0.0; // Neuordnung in %
    bool fragmentation = false; // Fragmentierung
    uint32_t fragment_size = 0; // Fragment-Größe in Bytes
  };

  // tc netem anwenden
  bool apply(const Config& config);
  
  // tc netem zurücksetzen
  bool reset();
  
  // tc netem-Status abrufen
  std::string get_status() const;
  
  // Prüfen, ob tc netem aktiv ist
  bool is_active() const { return active_; }
  
  // Root-Rechte prüfen
  static bool check_root();
  
  // tc netem-Test ausführen
  struct TestResult {
    double avg_latency_ms = 0.0;
    double min_latency_ms = 0.0;
    double max_latency_ms = 0.0;
    double jitter_ms = 0.0;
    double loss_percentage = 0.0;
    uint32_t packets_sent = 0;
    uint32_t packets_received = 0;
    uint32_t packets_dropped = 0;
  };

  TestResult run_test(const Config& config, int num_packets = 100);
  
private:
  bool active_ = false;
  Config current_config_;
  
  // Shell-Befehl ausführen
  std::string execute_command(const std::string& command) const;
  
  // tc netem-Regeln parsen
  Config parse_tc_output(const std::string& output) const;
};

} // namespace hh::client

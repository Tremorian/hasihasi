// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <csignal>
#include <memory>
#include <string>

#include "hh/logging.hpp"
#include "hh/config.hpp"

#include "hh/net/transport.hpp"

namespace {
volatile sig_atomic_t g_running = 1;

void signal_handler(int) {
  g_running = 0;
}
} // namespace

int main(int argc, char* argv[]) {
  std::string config_path = "/etc/hasihasi/server.toml";

  // Einfache CLI-Argumente
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--version") {
      std::cout << "hasihasi-server 0.1.0\n"
                   "SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>\n"
                   "SPDX-License-Identifier: GPL-3.0-or-later\n"
                   "Homepage: https://hasihasi.de\n";
      return 0;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Verwendung: hasihasi-server [OPTIONEN]\n"
                   "\n"
                   "Optionen:\n"
                   "  --config <pfad>   Konfigurationsdatei (Standard: /etc/hasihasi/server.toml)\n"
                   "  --version         Versionsinformation anzeigen\n"
                   "  --help, -h        Diese Hilfemeldung anzeigen\n";
      return 0;
    }
  }

  // Logging initialisieren
  hh::init_logging("/var/log/hasihasi/server.log");

  // Signale behandeln
  struct sigaction sa{};
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGINT, &sa, nullptr);

  // Konfiguration laden
  try {
    hh::Config config(config_path);
    (void)config;
  } catch (const std::exception& e) {
    std::cerr << "Fehler beim Laden der Konfiguration: " << e.what() << '\n';
    return 1;
  }

  hh::get_logger().info("HasiHasi-Server startet...");

  // TODO: Server-Logik implementieren (Meilenstein M2+)

  hh::get_logger().info("HasiHasi-Server gestoppt");
  return 0;
}

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <csignal>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include "hh/logging.hpp"
#include "hh/config.hpp"

#include "server/connection.hpp"
#include "server/voice_router.hpp"
#include "server/channel.hpp"
#include "server/user.hpp"

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace {
volatile sig_atomic_t g_running = 1;

void signal_handler(int) {
  g_running = 0;
}
} // namespace

// Handler für eingehende Nachrichten vom Control-Kanal
void handle_control_message(const std::shared_ptr<hh::server::Connection>& conn,
                            uint16_t type,
                            std::span<const uint8_t> payload) {
  (void)conn;
  (void)type;
  (void)payload;

  // TODO: Protobuf-Nachrichten parsen und verarbeiten
  // MSG_HELLO (0), MSG_AUTHENTICATE (2), MSG_CHANNEL_LIST (4), etc.
  hh::get_logger().debug("Control message received: type={}", type);
}

int main(int argc, char* argv[]) {
  std::string config_path = "/etc/hasihasi/server.toml";

  // Einfache CLI-Argumente
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--version") {
      std::cout << "hasihasi-server " << HH_VERSION << "\n"
                   "Build: " << HH_BUILD_TYPE << "\n"
                   "Build-Datum: " << HH_BUILD_DATE << "\n"
                   "\n"
                   "SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>\n"
                   "SPDX-License-Identifier: GPL-3.0-or-later\n"
                   "\n"
                   "HasiHasi – Low-Latency Linux Voice Chat\n"
                   "Homepage: https://hasihasi.de\n"
                   "GitHub: https://github.com/tremorian/hasihasi\n";
      return 0;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Verwendung: hasihasi-server [OPTIONEN]\n"
                   "\n"
                   "HasiHasi – Low-Latency Linux Voice Chat\n"
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

  // ── Asio-IO-Context ─────────────────────────────────────────────────
  asio::io_context ioc;

  // ── TLS-Server ───────────────────────────────────────────────────────
  asio::ssl::context ssl_context(asio::ssl::context::sslv23);

  // TODO: Zertifikat und Key laden (aus Konfiguration)
  // ssl_context.use_certificate_chain_file("/etc/hasihasi/cert.pem");
  // ssl_context.use_private_key_file("/etc/hasihasi/key.pem", asio::ssl::context::pem);

  // Für Testzwecke: Self-Signed-Zertifikat generieren (TOFU)
  hh::get_logger().warn("Kein TLS-Zertifikat geladen – Server startet mit Self-Signed-Zertifikat");

  // ── TCP-Listener ─────────────────────────────────────────────────────
  asio::ip::tcp::acceptor acceptor(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 64738));
  hh::get_logger().info("TLS-Control-Listener auf Port 64738");

  // ── Voice-Router ─────────────────────────────────────────────────────
  hh::server::VoiceRouter voice_router(ioc);
  if (!voice_router.open(64738)) {
    hh::get_logger().error("Voice-Router konnte nicht geöffnet werden");
    return 1;
  }
  hh::get_logger().info("Voice-UDP auf Port {}", voice_router.local_endpoint().port());

  // ── Kanalbaum ────────────────────────────────────────────────────────
  hh::server::ChannelTree channel_tree;

  // Root-Kanal + Test-Kanal
  channel_tree.create_channel(hh::server::ChannelTree::ROOT_CHANNEL_ID, "Lobby", 50, false, "Hauptkanal");
  channel_tree.create_channel(hh::server::ChannelTree::ROOT_CHANNEL_ID, "Test", 10, true, "Testkanal");

  // ── Nutzer-Verwaltung ────────────────────────────────────────────────
  hh::server::UserManager user_manager;

  // ── Verbindungen akzeptieren ─────────────────────────────────────────
  hh::get_logger().info("Bereit für Verbindungen (STRG+C zum Stoppen)");

  while (g_running) {
    // Asio-Events verarbeiten
    ioc.poll();

    // Neue Verbindung akzeptieren
    try {
      asio::ip::tcp::socket socket(ioc);
      acceptor.accept(socket);

      hh::get_logger().info("Neue Verbindung von {}", socket.remote_endpoint().address().to_string());

      // TLS-Stream erstellen
      auto ssl_socket = std::make_shared<asio::ssl::stream<asio::ip::tcp::socket>>(
        std::move(socket), ssl_context);

      // Connection erstellen
      auto connection = std::make_shared<hh::server::Connection>(*ssl_socket);

      // Connection starten
      connection->start([connection](uint16_t type, std::span<const uint8_t> payload) {
        handle_control_message(connection, type, payload);
      });

    } catch (const std::exception& e) {
      hh::get_logger().error("Fehler beim Akzeptieren: {}", e.what());
    }

    // Schlafen, um CPU zu sparen
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  hh::get_logger().info("HasiHasi-Server gestoppt");
  return 0;
}

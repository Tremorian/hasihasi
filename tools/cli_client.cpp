// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

// hasihasi-cli – Testclient für den HasiHasi-Server
//
// Verwendung:
//   hasihasi-cli connect <host>:<port> <username>
//   hasihasi-cli list-channels
//   hasihasi-cli list-users
//   hasihasi-cli send-text <channel_id|user_id> <text>
//   hasihasi-cli version
//
// Dieser Client testet den Control-Kanal (TLS) und kann sich mit einem
// anderen Client gegenseitig hören (über UDP-Voice).

#include <iostream>
#include <string>
#include <csignal>
#include <vector>
#include <chrono>

#include "hh/logging.hpp"
#include "hh/proto/framing.hpp"

// Forward-Deklarationen für Asio/SSL
#include <asio.hpp>
#include <asio/ssl.hpp>

namespace {
volatile sig_atomic_t g_running = 1;

void signal_handler(int) {
  g_running = 0;
}

// Einfache TLS-Verbindung zum Server
class ClientConnection {
public:
  explicit ClientConnection(asio::io_context& ioc)
    : ioc_(ioc)
    , ssl_context_(asio::ssl::context::sslv23)
    , socket_(ioc_, ssl_context_) {
    // Zertifizierungsprüfung deaktivieren (für Testzwecke)
    ssl_context_.set_verify_mode(asio::ssl::verify_none);
  }

  bool connect(const std::string& host, uint16_t port) {
    // Resolver
    asio::ip::tcp::resolver resolver(ioc_);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    // Connect
    auto ec = asio::connect(socket_.next_layer(), endpoints);
    if (ec) {
      std::cerr << "Connect failed: " << ec.message() << '\n';
      return false;
    }

    // TLS-Handshake
    socket_.handshake(asio::ssl::stream_base::client);

    std::cout << "Verbunden mit " << host << ":" << port << '\n';
    return true;
  }

  void send_message(uint16_t type, const std::vector<uint8_t>& payload) {
    auto frame = hh::proto::FrameBuilder::build(type, payload);
    asio::write(socket_, asio::buffer(frame));
  }

  void send_hello(uint32_t protocol_version, const std::string& client_version) {
    // Einfache Hello-Nachricht als rohe Bytes
    std::vector<uint8_t> data;
    data.resize(4 + 4 + client_version.size());

    // protocol_version (u32)
    data[0] = (protocol_version >> 24) & 0xFF;
    data[1] = (protocol_version >> 16) & 0xFF;
    data[2] = (protocol_version >> 8) & 0xFF;
    data[3] = protocol_version & 0xFF;

    // client_version length (u32)
    uint32_t len = static_cast<uint32_t>(client_version.size());
    data[4] = (len >> 24) & 0xFF;
    data[5] = (len >> 16) & 0xFF;
    data[6] = (len >> 8) & 0xFF;
    data[7] = len & 0xFF;

    // client_version
    std::copy(client_version.begin(), client_version.end(), data.begin() + 8);

    send_message(0, data); // MSG_HELLO
  }

  bool is_connected() const {
    return socket_.is_open();
  }

  void close() {
    std::error_code ec;
    socket_.shutdown(ec);
    socket_.close(ec);
  }

  asio::ssl::stream<asio::ip::tcp::socket>& socket() {
    return socket_;
  }

private:
  asio::io_context& ioc_;
  asio::ssl::context ssl_context_;
  asio::ssl::stream<asio::ip::tcp::socket> socket_;
};

void print_usage() {
  std::cout << "HasiHasi CLI Testclient\n"
               "\n"
               "Verwendung:\n"
               "  hasihasi-cli connect <host>:<port> <username>     Mit Server verbinden\n"
               "  hasihasi-cli list-channels                        Kanäle auflisten\n"
               "  hasihasi-cli list-users                           Nutzer auflisten\n"
               "  hasihasi-cli send-text <target_id> <text>         Textnachricht senden\n"
               "  hasihasi-cli version                              Versionsinformation\n"
               "\n"
               "Beispiel:\n"
               "  hasihasi-cli connect localhost:64738 TestUser\n";
}

} // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  std::string cmd = argv[1];

  if (cmd == "version") {
    std::cout << "hasihasi-cli 0.1.0\n"
                 "SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>\n"
                 "SPDX-License-Identifier: GPL-3.0-or-later\n";
    return 0;
  }

  if (cmd == "help" || cmd == "-h" || cmd == "--help") {
    print_usage();
    return 0;
  }

  if (cmd == "connect" && argc >= 5) {
    // Verbindung zum Server herstellen
    std::string host_port = argv[2];
    std::string username = argv[4];

    // Host und Port parsen
    auto colon_pos = host_port.find(':');
    if (colon_pos == std::string::npos) {
      std::cerr << "Ungültiges Format. Verwende host:port\n";
      return 1;
    }

    std::string host = host_port.substr(0, colon_pos);
    uint16_t port = static_cast<uint16_t>(std::stoi(host_port.substr(colon_pos + 1)));

    // Logging
    hh::init_logging("", true);

    // Signal-Handling
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Asio-IO-Context
    asio::io_context ioc;

    // Verbindung erstellen
    ClientConnection conn(ioc);

    if (!conn.connect(host, port)) {
      return 1;
    }

    // Hello senden
    conn.send_hello(1, "hasihasi-cli/0.1.0");

    std::cout << "Verbunden als: " << username << '\n';
    std::cout << "Tippe 'quit' zum Beenden, 'help' für Hilfe.\n";

    // Einfache Interaktion (nicht-asynchron für Testzwecke)
    while (g_running && conn.is_connected()) {
      std::string input;
      std::getline(std::cin, input);

      if (input == "quit" || input == "exit") {
        break;
      }

      if (input == "help") {
        std::cout << "Befehle: quit, help, status\n";
        continue;
      }

      if (input == "status") {
        std::cout << "Status: verbunden\n";
        continue;
      }

      // Hier könnten weitere Befehle verarbeitet werden
      (void)username;
    }

    conn.close();
    std::cout << "Verbindung getrennt.\n";

  } else if (cmd == "list-channels" || cmd == "list-users") {
    std::cout << "Befehl nicht implementiert (benötigt aktive Verbindung)\n";
    std::cout << "Verwende: hasihasi-cli connect <host>:<port> <username>\n";

  } else if (cmd == "send-text" && argc >= 4) {
    std::cout << "Befehl nicht implementiert (benötigt aktive Verbindung)\n";
    std::cout << "Verwende: hasihasi-cli connect <host>:<port> <username>\n";

  } else {
    std::cerr << "Unbekannter Befehl: " << cmd << '\n';
    print_usage();
    return 1;
  }

  return 0;
}

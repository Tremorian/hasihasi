// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <string>

// hasihasi-cli – Admin-Werkzeug für den Server
// TODO: Implementierung (Meilenstein M5+)

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Verwendung: hasihasi-cli <befehl> [optionen]\n"
                 "\n"
                 "Befehle:\n"
                 "  connect <host>[:port]   Mit Server verbinden\n"
                 "  list-channels           Kanäle auflisten\n"
                 "  list-users              Nutzer auflisten\n"
                 "  kick <user> [grund]     Nutzer kicken\n"
                 "  ban <ip|user> [dauer]   Nutzer/IP bannen\n"
                 "  version                 Versionsinformation\n";
    return 1;
  }

  std::string cmd = argv[1];

  if (cmd == "version") {
    std::cout << "hasihasi-cli 0.1.0\n"
                 "SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>\n"
                 "SPDX-License-Identifier: GPL-3.0-or-later\n";
    return 0;
  }

  std::cerr << "Befehl nicht implementiert: " << cmd << '\n';
  return 1;
}

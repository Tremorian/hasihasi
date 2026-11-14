// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#define CATCH2_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

// Basis-Test – bestätigt, dass Catch2 funktioniert
TEST_CASE("Catch2 funktioniert", "[setup]") {
  REQUIRE(1 + 1 == 2);
}

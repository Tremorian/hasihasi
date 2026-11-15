# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(hasihasi-cli
  main.cpp
)

target_link_libraries(hasihasi-cli
  PRIVATE
    hh_common::hh_common
    hh_net::hh_net
)

target_compile_features(hasihasi-cli PRIVATE cxx_std_20)

install(TARGETS hasihasi-cli
  RUNTIME DESTINATION bin
)

# ── Loopback-Testprogramm ─────────────────────────────────────────────

add_executable(hasihasi-loopback
  loopback.cpp
)

target_link_libraries(hasihasi-loopback
  PRIVATE
    hh_common::hh_common
    hh_audio::hh_audio
)

target_compile_features(hasihasi-loopback PRIVATE cxx_std_20)

install(TARGETS hasihasi-loopback
  RUNTIME DESTINATION bin
)

# ── CLI-Testclient ────────────────────────────────────────────────────

add_executable(hasihasi-cli-test
  cli_client.cpp
)

target_link_libraries(hasihasi-cli-test
  PRIVATE
    hh_common::hh_common
    hh_proto::hh_proto
    ${OPENSSL_LIBRARIES}
)

target_compile_features(hasihasi-cli-test PRIVATE cxx_std_20)

install(TARGETS hasihasi-cli-test
  RUNTIME DESTINATION bin
)

# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Catch2 3 REQUIRED)

add_executable(unit_tests
  main.cpp
  test_framing.cpp
  test_ringbuffer.cpp
)

target_link_libraries(unit_tests
  PRIVATE
    Catch2::Catch2WithMain
    hh_proto::hh_proto
    hh_audio::hh_audio
)

target_compile_features(unit_tests PRIVATE cxx_std_20)

catch_discover_tests(unit_tests)

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

# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(hasihasi-server
  main.cpp
)

target_link_libraries(hasihasi-server
  PRIVATE
    hh_common::hh_common
    hh_net::hh_net
)

install(TARGETS hasihasi-server
  RUNTIME DESTINATION bin
)

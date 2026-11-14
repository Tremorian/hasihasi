# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(SQLite3 REQUIRED)

add_executable(hasihasi-server
  main.cpp
  connection.cpp
  voice_router.cpp
  channel.cpp
  user.cpp
  session_manager.cpp
  acl.cpp
  database.cpp
)

target_include_directories(hasihasi-server
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(hasihasi-server
  PRIVATE
    hh_common::hh_common
    hh_proto::hh_proto
    hh_net::hh_net
    OpenSSL::TLS
    SQLite::SQLite3
)

install(TARGETS hasihasi-server
  RUNTIME DESTINATION bin
)

# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(SQLite3 REQUIRED)

# Server library (for tests)
add_library(hh_server_lib
  voice_router.cpp
  voice_crypto.cpp
  cert_auth.cpp
  prometheus_metrics.cpp
  admin_server.cpp
  rate_limiter.cpp
  welcome_message.cpp
  channel.cpp
  user.cpp
  session_manager.cpp
  acl.cpp
  database.cpp
  text_chat.cpp
  ban_kick.cpp
  whisper_shout.cpp
  server_i18n.cpp
  server_cluster.cpp
  server_discovery.cpp
  load_balancer.cpp
)

target_include_directories(hh_server_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(hh_server_lib
  PUBLIC
    hh_common::hh_common
    hh_proto::hh_proto
    hh_net::hh_net
    OpenSSL::TLS
    SQLite::SQLite3
)

# Server executable
add_executable(hasihasi-server
  main.cpp
)

# Versionsinformationen
set(HH_VERSION "0.1.0")
set(HH_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
string(TIMESTAMP HH_BUILD_DATE "%Y-%m-%d %H:%M:%S")

target_compile_definitions(hasihasi-server
  PRIVATE
    HH_VERSION="${HH_VERSION}"
    HH_BUILD_TYPE="${HH_BUILD_TYPE}"
    HH_BUILD_DATE="${HH_BUILD_DATE}"
)

target_link_libraries(hasihasi-server
  PRIVATE
    hh_server_lib
)

install(TARGETS hasihasi-server
  RUNTIME DESTINATION bin
)

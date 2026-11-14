# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(hh_net
  src/transport.cpp
)

add_library(hh_net::hh_net ALIAS hh_net)

target_include_directories(hh_net
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(hh_net
  PUBLIC
    hh_common::hh_common
    hh_proto::hh_proto
    asio::asio
  PRIVATE
    OpenSSL::TLS
    Threads::Threads
)

target_compile_features(hh_net PUBLIC cxx_std_20)

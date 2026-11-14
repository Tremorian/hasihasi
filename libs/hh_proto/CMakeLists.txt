# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(hh_proto
  src/framing.cpp
)

add_library(hh_proto::hh_proto ALIAS hh_proto)

target_include_directories(hh_proto
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(hh_proto PUBLIC cxx_std_20)

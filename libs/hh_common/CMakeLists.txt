# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(hh_common
  src/config.cpp
)

add_library(hh_common::hh_common ALIAS hh_common)

target_include_directories(hh_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(hh_common
  PUBLIC
    spdlog::spdlog
    tomlplusplus::tomlplusplus
  PRIVATE
    Threads::Threads
)

target_compile_features(hh_common PUBLIC cxx_std_20)

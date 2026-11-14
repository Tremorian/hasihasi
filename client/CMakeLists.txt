# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# ── Client Core (ohne Qt) ─────────────────────────────────────────────

add_library(client_core
  src/session.cpp
)

add_library(client_core::client_core ALIAS client_core)

target_include_directories(client_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(client_core
  PUBLIC
    hh_common::hh_common
    hh_net::hh_net
)

target_compile_features(client_core PUBLIC cxx_std_20)

# ── Qt-GUI ────────────────────────────────────────────────────────────

add_executable(hasihasi
  ui/src/mainwindow.cpp
  ui/src/statisticswidget.cpp
)

target_link_libraries(hasihasi
  PRIVATE
    Qt6::Widgets
    client_core::client_core
    hh_common::hh_common
)

target_compile_features(hasihasi PRIVATE cxx_std_20)

# Resources (Platzhalter)
qt_add_resources(hasihasi "resources"
  PREFIX "/"
  FILES
    resources/logo_placeholder.svg
)

install(TARGETS hasihasi
  RUNTIME DESTINATION bin
)

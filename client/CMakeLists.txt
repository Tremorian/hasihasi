# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# ── Client Core (ohne Qt) ─────────────────────────────────────────────

add_library(client_core
  core/src/session.cpp
  core/src/voice_pipeline.cpp
  core/src/latency_test.cpp
  core/src/global_hotkey_manager.cpp
  core/src/system_tray.cpp
  core/src/audio_assistant.cpp
  core/src/server_list.cpp
  core/src/echo_cancellation.cpp
  core/src/game_overlay.cpp
  core/src/positional_audio_plugin.cpp
  core/src/dbus_interface.cpp
  core/src/autostart_manager.cpp
  core/src/theme_manager.cpp
  core/src/hidpi_manager.cpp
  core/src/markdown_light.cpp
  core/src/accessibility.cpp
  core/src/dsp_thread.cpp
  core/src/tc_netem_test.cpp
  core/src/image_manager.cpp
  core/src/i18n_manager.cpp
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
    Qt6::DBus
)

target_compile_features(client_core PUBLIC cxx_std_20)

# ── Qt-GUI ────────────────────────────────────────────────────────────

add_executable(hasihasi
  ui/src/mainwindow.cpp
  ui/src/statisticswidget.cpp
  ui/src/aboutdialog.cpp
  ui/src/languagedialog.cpp
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

# Translation-Resources
qt_add_resources(hasihasi "translations"
  PREFIX "/"
  FILES
    resources/translations.qrc
)

install(TARGETS hasihasi
  RUNTIME DESTINATION bin
)

# ── Mobile-Client (Qt Quick) ─────────────────────────────────────────

option(HASIHASI_BUILD_MOBILE "Build mobile client (Qt Quick)" OFF)

if(HASIHASI_BUILD_MOBILE)
  add_subdirectory(mobile)
endif()

# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(hh_audio
  src/buffers.cpp
  src/pipewire.cpp
)

add_library(hh_audio::hh_audio ALIAS hh_audio)

target_include_directories(hh_audio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(hh_audio
  PUBLIC
    hh_common::hh_common
  PRIVATE
    PipeWire::PipeWire
    opus
    Threads::Threads
)

target_compile_features(hh_audio PUBLIC cxx_std_20)

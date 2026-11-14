# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

# RNNoise als Submodule
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/rnnoise)

add_library(hh_audio
  src/buffers.cpp
  src/pipewire.cpp
  src/jitter_buffer.cpp
  src/fec_plc.cpp
  src/vad.cpp
  src/noise_suppression.cpp
  src/positional_audio.cpp
)

add_library(hh_audio::hh_audio ALIAS hh_audio)

target_include_directories(hh_audio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus>=1.4)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3>=0.3.27)

target_include_directories(hh_audio
  PRIVATE
    ${OPUS_INCLUDE_DIRS}
    ${PIPEWIRE_INCLUDE_DIRS}
)

target_link_libraries(hh_audio
  PUBLIC
    hh_common::hh_common
  PRIVATE
    ${OPUS_LIBRARIES}
    ${PIPEWIRE_LIBRARIES}
    Threads::Threads
    rnnoise
)

target_compile_options(hh_audio
  PRIVATE
    ${OPUS_CFLAGS_OTHER}
    ${PIPEWIRE_CFLAGS_OTHER}
)

target_compile_features(hh_audio PUBLIC cxx_std_20)

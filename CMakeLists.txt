# HasiHasi – Low-Latency-Voice-Chat für Linux-Gamer
# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(hasihasi
  VERSION 0.1.0
  DESCRIPTION "Low-Latency-Voice-Chat für Linux-Gamer"
  HOMEPAGE_URL "https://hasihasi.de"
  LANGUAGES CXX
)

# ── Global-Einstellungen ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Standard-CXX-Flags: streng, aber portabel
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Debug-Symbole immer mitbauen (für Profiling/Debugging)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE CACHE STRING "Build type" FORCE)
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# Export compile_commands.json für IDE/clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Optionen ──────────────────────────────────────────────────────────
option(HASIHASI_BUILD_SERVER   "Baut den hasihasi-Server"   ON)
option(HASIHASI_BUILD_CLIENT  "Baut den Qt6-Client"        ON)
option(HASIHASI_BUILD_TOOLS   "Baut hasihasi-cli"          ON)
option(HASIHASI_BUILD_TESTS   "Baut Tests"                 ON)

# ── Abhängigkeiten (systemweit) ───────────────────────────────────────
find_package(Threads REQUIRED)

# ── Unterverzeichnisse ────────────────────────────────────────────────
add_subdirectory(libs/hh_common)
add_subdirectory(libs/hh_proto)
add_subdirectory(libs/hh_net)
add_subdirectory(libs/hh_audio)

if(HASIHASI_BUILD_SERVER)
  add_subdirectory(server)
endif()

if(HASIHASI_BUILD_CLIENT)
  add_subdirectory(client)
endif()

if(HASIHASI_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(HASIHASI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

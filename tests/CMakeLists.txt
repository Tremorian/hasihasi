# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Catch2 3 REQUIRED)

add_executable(unit_tests
  main.cpp
  test_framing.cpp
  test_ringbuffer.cpp
  test_loopback.cpp
  test_server.cpp
  test_client.cpp
  test_jitter_buffer.cpp
  test_positonal_audio.cpp
  test_session.cpp
  test_voice_pipeline.cpp
  test_latency_test.cpp
  test_reconnect_manager.cpp
  test_acl.cpp
  test_database.cpp
  test_user_cert.cpp
  test_text_chat.cpp
  test_ban_kick.cpp
  test_whisper_shout.cpp
  test_voice_crypto.cpp
)

target_link_libraries(unit_tests
  PRIVATE
    Catch2::Catch2WithMain
    hh_proto::hh_proto
    hh_audio::hh_audio
    hh_net::hh_net
    client_core::client_core
    hh_server_lib
)

target_compile_features(unit_tests PRIVATE cxx_std_20)

catch_discover_tests(unit_tests)

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
  test_jitter_buffer_time_stretch.cpp
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
  test_cert_auth.cpp
  test_server_password_auth.cpp
  test_protocol_version.cpp
  test_voice_router_benchmark.cpp
  test_prometheus_metrics.cpp
  test_admin_server.cpp
  test_rate_limiter.cpp
  test_welcome_message.cpp
  test_logging.cpp
  test_dbus.cpp
  test_autostart.cpp
  test_theme.cpp
  test_hidpi.cpp
  test_voice_router.cpp
  test_about_dialog.cpp
  test_markdown.cpp
  test_accessibility.cpp
  test_dsp_thread.cpp
  test_tc_netem.cpp
  test_image_manager.cpp
  test_i18n_manager.cpp
  test_push_notification.cpp
  test_mobile_audio.cpp
  test_offline_cache.cpp
  test_battery_optimizer.cpp
  test_voice_assistant.cpp
  test_file_share.cpp
  test_screen_share.cpp
  test_monitoring.cpp
  test_advanced_server.cpp
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

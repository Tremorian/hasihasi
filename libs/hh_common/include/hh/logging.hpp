// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace hh {

inline void init_logging(const std::string& log_file, bool console = true) {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

  if (console) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    spdlog::stdout_logger_mt("console") = console_sink;
  }

  if (!log_file.empty()) {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
    spdlog::create<spdlog::sinks::basic_file_sink_mt>("file", log_file);
  }
}

inline auto& get_logger() {
  return *spdlog::default_logger();
}

} // namespace hh

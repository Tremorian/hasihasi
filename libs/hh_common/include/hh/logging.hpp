// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/pattern_formatter.h>
#include <syslog.h>

#include <string>
#include <memory>

namespace hh {

// journald-Kompatibler Logger
// Sendet strukturierte Logs nach systemd-journald über SYSLOG_IDENTIFIER
class journald_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
  explicit journald_sink(const std::string& identifier = "hasihasi-server")
    : identifier_(identifier) {
    // SYSLOG_IDENTIFIER setzen
    setenv("SYSLOG_IDENTIFIER", identifier.c_str(), 1);
  }

  ~journald_sink() override = default;

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    // Log-Level mapping
    int priority = LOG_INFO;
    switch (msg.level) {
      case spdlog::level::trace:
        priority = LOG_DEBUG;
        break;
      case spdlog::level::debug:
        priority = LOG_DEBUG;
        break;
      case spdlog::level::info:
        priority = LOG_INFO;
        break;
      case spdlog::level::warn:
        priority = LOG_WARNING;
        break;
      case spdlog::level::err:
        priority = LOG_ERR;
        break;
      case spdlog::level::critical:
        priority = LOG_CRIT;
        break;
      case spdlog::level::off:
        priority = LOG_INFO;
        break;
    }

    // Nachricht formatieren
    std::string formatted_message;
    if (msg.logger_name != nullptr) {
      formatted_message = "[" + std::string(msg.logger_name) + "] " + msg.payload;
    } else {
      formatted_message = msg.payload;
    }

    // Nach journald senden
    syslog(priority, "%s", formatted_message.c_str());
  }

  void flush_() override {
    // syslog ist synchron, kein Flush nötig
  }

  void set_pattern(const std::string& pattern) override {
    // Pattern wird vom Logger gesetzt, hier ignorieren
    (void)pattern;
  }

private:
  std::string identifier_;
};

// Logger-Initialisierung mit journald-Unterstützung
inline void init_logging(const std::string& log_file, bool console = true, bool journald = false) {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");

  // Console-Sink
  if (console) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto console_logger = std::make_shared<spdlog::logger>("console", console_sink);
    spdlog::register_logger(console_logger);
  }

  // File-Sink
  if (!log_file.empty()) {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
    auto file_logger = std::make_shared<spdlog::logger>("file", file_sink);
    spdlog::register_logger(file_logger);
  }

  // journald-Sink
  if (journald) {
    auto journald_sink_ptr = std::make_shared<journald_sink>("hasihasi-server");
    auto journald_logger = std::make_shared<spdlog::logger>("journald", journald_sink_ptr);
    spdlog::register_logger(journald_logger);
  }

  // Default-Logger setzen
  auto default_logger = spdlog::get("console");
  if (default_logger) {
    spdlog::set_default_logger(default_logger);
  }
}

// Logger mit journald-Unterstützung
inline void init_logging_with_journald(const std::string& log_file = "", bool console = true) {
  init_logging(log_file, console, true);
}

inline auto& get_logger() {
  return *spdlog::default_logger();
}

// Strukturierte Logging-Hilfsfunktionen
inline void log_structured(const std::string& key, const std::string& value,
                           spdlog::level::level_enum level = spdlog::level::info) {
  auto logger = spdlog::default_logger();
  if (!logger) return;

  std::string message = key + "=" + value;
  
  switch (level) {
    case spdlog::level::trace:
      logger->trace(message);
      break;
    case spdlog::level::debug:
      logger->debug(message);
      break;
    case spdlog::level::info:
      logger->info(message);
      break;
    case spdlog::level::warn:
      logger->warn(message);
      break;
    case spdlog::level::err:
      logger->error(message);
      break;
    case spdlog::level::critical:
      logger->critical(message);
      break;
    case spdlog::level::off:
      break;
  }
}

} // namespace hh

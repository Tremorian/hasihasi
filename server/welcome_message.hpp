// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace hh::server {

// Willkommensnachricht bei Kanalbeitritt
class WelcomeMessage {
public:
  // Konfiguration für Willkommensnachricht
  struct Config {
    bool enabled = true;
    std::string channel_welcome_template; // {username} wird ersetzt
    std::string server_welcome_template;
    bool send_on_channel_join = true;
    bool send_on_server_join = true;
    bool include_channel_description = true;
    bool include_user_count = true;
  };

  WelcomeMessage() = default;
  ~WelcomeMessage() = default;

  // Initialisierung
  bool initialize(const Config& config);
  void shutdown();

  // Willkommensnachricht generieren
  struct WelcomeInfo {
    std::string username;
    uint32_t user_id;
    uint32_t channel_id;
    std::string channel_name;
    std::string channel_description;
    size_t channel_user_count;
    size_t total_user_count;
    bool is_first_join;
  };

  std::string generate_welcome_message(const WelcomeInfo& info);
  std::string generate_channel_welcome(const WelcomeInfo& info);
  std::string generate_server_welcome(const WelcomeInfo& info);

  // Konfiguration
  void set_config(const Config& config);
  Config config() const { return config_; }

  // Template setzen/ändern
  bool set_channel_template(const std::string& template_str);
  bool set_server_template(const std::string& template_str);

  // Statistik
  struct Stats {
    size_t messages_sent;
    size_t channel_joins;
    size_t server_joins;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using WelcomeSentCallback = std::function<void(uint32_t user_id, const std::string& message)>;
  void set_welcome_sent_callback(WelcomeSentCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  Config config_;
  Stats stats_;
  WelcomeSentCallback welcome_sent_callback_;
};

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/welcome_message.hpp"
#include "hh/logging.hpp"

#include <sstream>
#include <algorithm>

namespace hh::server {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct WelcomeMessage::Impl {
  Config config_;
  
  Impl() = default;
  ~Impl() = default;

  bool initialize(const Config& config) {
    config_ = config;
    
    // Default-Templates setzen, falls nicht angegeben
    if (config_.channel_welcome_template.empty()) {
      config_.channel_welcome_template = "Willkommen {username} im Kanal {channel_name}!";
    }
    if (config_.server_welcome_template.empty()) {
      config_.server_welcome_template = "Willkommen auf dem HasiHasi-Server, {username}!";
    }

    hh::get_logger().info("WelcomeMessage: Initialized with config (enabled: {}, channel_template: {})",
                          config_.enabled, config_.channel_welcome_template);
    return true;
  }

  void shutdown() {
    hh::get_logger().info("WelcomeMessage: Shutdown");
  }

  std::string replace_template(const std::string& template_str, const WelcomeInfo& info) {
    std::string result = template_str;
    
    // {username} ersetzen
    size_t pos = result.find("{username}");
    while (pos != std::string::npos) {
      result.replace(pos, 10, info.username);
      pos = result.find("{username}", pos + info.username.length());
    }
    
    // {channel_name} ersetzen
    pos = result.find("{channel_name}");
    while (pos != std::string::npos) {
      result.replace(pos, 14, info.channel_name);
      pos = result.find("{channel_name}", pos + info.channel_name.length());
    }
    
    // {channel_description} ersetzen
    pos = result.find("{channel_description}");
    while (pos != std::string::npos) {
      result.replace(pos, 23, info.channel_description);
      pos = result.find("{channel_description}", pos + info.channel_description.length());
    }
    
    // {user_count} ersetzen
    pos = result.find("{user_count}");
    while (pos != std::string::npos) {
      std::string count_str = std::to_string(info.channel_user_count);
      result.replace(pos, 12, count_str);
      pos = result.find("{user_count}", pos + count_str.length());
    }
    
    // {total_users} ersetzen
    pos = result.find("{total_users}");
    while (pos != std::string::npos) {
      std::string count_str = std::to_string(info.total_user_count);
      result.replace(pos, 13, count_str);
      pos = result.find("{total_users}", pos + count_str.length());
    }
    
    // {is_first_join} ersetzen
    pos = result.find("{is_first_join}");
    while (pos != std::string::npos) {
      std::string join_str = info.is_first_join ? "Ja" : "Nein";
      result.replace(pos, 15, join_str);
      pos = result.find("{is_first_join}", pos + join_str.length());
    }
    
    return result;
  }

  std::string generate_welcome_message(const WelcomeInfo& info) {
    if (!config_.enabled) {
      return "";
    }

    if (info.is_first_join) {
      return generate_server_welcome(info);
    } else {
      return generate_channel_welcome(info);
    }
  }

  std::string generate_channel_welcome(const WelcomeInfo& info) {
    if (!config_.enabled || !config_.send_on_channel_join) {
      return "";
    }

    std::string template_str = config_.channel_welcome_template;
    
    // Zusätzliche Informationen einfügen
    if (config_.include_channel_description && !info.channel_description.empty()) {
      template_str += "\nBeschreibung: " + info.channel_description;
    }
    if (config_.include_user_count) {
      template_str += "\nNutzer in diesem Kanal: " + std::to_string(info.channel_user_count);
    }

    return replace_template(template_str, info);
  }

  std::string generate_server_welcome(const WelcomeInfo& info) {
    if (!config_.enabled || !config_.send_on_server_join) {
      return "";
    }

    return replace_template(config_.server_welcome_template, info);
  }
};

// ── WelcomeMessage ────────────────────────────────────────────────────

bool WelcomeMessage::initialize(const Config& config) {
  impl_ = std::make_unique<Impl>();
  return impl_->initialize(config);
}

void WelcomeMessage::shutdown() {
  if (impl_) {
    impl_->shutdown();
  }
}

std::string WelcomeMessage::generate_welcome_message(const WelcomeInfo& info) {
  if (!impl_) {
    return "";
  }
  return impl_->generate_welcome_message(info);
}

std::string WelcomeMessage::generate_channel_welcome(const WelcomeInfo& info) {
  if (!impl_) {
    return "";
  }
  return impl_->generate_channel_welcome(info);
}

std::string WelcomeMessage::generate_server_welcome(const WelcomeInfo& info) {
  if (!impl_) {
    return "";
  }
  return impl_->generate_server_welcome(info);
}

void WelcomeMessage::set_config(const Config& config) {
  config_ = config;
  if (impl_) {
    impl_->set_config(config);
  }
}

WelcomeMessage::Config WelcomeMessage::config() const {
  return config_;
}

bool WelcomeMessage::set_channel_template(const std::string& template_str) {
  config_.channel_welcome_template = template_str;
  if (impl_) {
    impl_->config_.channel_welcome_template = template_str;
  }
  hh::get_logger().info("WelcomeMessage: Channel template set to: {}", template_str);
  return true;
}

bool WelcomeMessage::set_server_template(const std::string& template_str) {
  config_.server_welcome_template = template_str;
  if (impl_) {
    impl_->config_.server_welcome_template = template_str;
  }
  hh::get_logger().info("WelcomeMessage: Server template set to: {}", template_str);
  return true;
}

WelcomeMessage::Stats WelcomeMessage::get_stats() const {
  return stats_;
}

void WelcomeMessage::reset_stats() {
  stats_ = {};
}

void WelcomeMessage::set_welcome_sent_callback(WelcomeSentCallback callback) {
  welcome_sent_callback_ = callback;
}

} // namespace hh::server

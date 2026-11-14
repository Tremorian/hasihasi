// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <chrono>

namespace hh::server {

// Bot-System für den HasiHasi-Server
class BotManager {
public:
  struct Bot {
    uint32_t bot_id;
    std::string name;
    std::string description;
    std::string avatar_url;
    bool is_active = true;
    bool is_owner = false;
    uint32_t owner_id = 0;
    double created_at = 0.0;
    double last_activity = 0.0;
    
    // Bot-Konfiguration
    std::vector<std::string> commands;
    std::unordered_map<std::string, std::string> permissions;
    std::unordered_map<std::string, std::string> settings;
  };

  BotManager() = default;
  ~BotManager() = default;

  // Nicht kopierbar, bewegbar
  BotManager(const BotManager&) = delete;
  BotManager& operator=(const BotManager&) = delete;
  BotManager(BotManager&&) noexcept;
  BotManager& operator=(BotManager&&) noexcept;

  // Bot-Verwaltung
  bool add_bot(const Bot& bot);
  bool remove_bot(uint32_t bot_id);
  Bot* get_bot(uint32_t bot_id);
  const Bot* get_bot(uint32_t bot_id) const;
  Bot* get_bot_by_name(const std::string& name);
  std::vector<Bot> get_all_bots() const;
  std::vector<Bot> get_active_bots() const;

  // Bot-Aktivität
  void activate_bot(uint32_t bot_id);
  void deactivate_bot(uint32_t bot_id);
  void update_bot_activity(uint32_t bot_id);

  // Bot-Befehle
  using CommandCallback = std::function<bool(const std::string& command, const std::string& args, uint32_t sender_id)>;
  
  bool register_command(uint32_t bot_id, const std::string& command, CommandCallback callback);
  bool unregister_command(uint32_t bot_id, const std::string& command);
  bool execute_command(uint32_t bot_id, const std::string& command, const std::string& args, uint32_t sender_id);

  // Bot-Berechtigungen
  bool set_bot_permission(uint32_t bot_id, const std::string& permission, const std::string& value);
  std::string get_bot_permission(uint32_t bot_id, const std::string& permission) const;
  bool has_bot_permission(uint32_t bot_id, const std::string& permission) const;

  // Bot-Einstellungen
  bool set_bot_setting(uint32_t bot_id, const std::string& key, const std::string& value);
  std::string get_bot_setting(uint32_t bot_id, const std::string& key) const;

  // Bot-Integration
  using BotResponseCallback = std::function<void(uint32_t bot_id, const std::string& response)>;
  
  void set_bot_response_callback(BotResponseCallback callback);
  void send_bot_response(uint32_t bot_id, const std::string& response);

  // Statistiken
  struct BotStats {
    uint32_t total_bots = 0;
    uint32_t active_bots = 0;
    uint32_t inactive_bots = 0;
    uint32_t total_commands = 0;
    uint32_t total_responses = 0;
  };

  BotStats get_stats() const;
  void reset_stats();

private:
  // Bot-Thread
  void bot_thread_func(uint32_t bot_id);
  
  // Bot-Scheduler
  void scheduler_thread_func();
  
  // Einstellungen
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, Bot> bots_;
  uint32_t next_bot_id_ = 1;
  
  // Befehle
  std::unordered_map<uint32_t, std::unordered_map<std::string, CommandCallback>> commands_;
  
  // Callbacks
  BotResponseCallback bot_response_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> total_commands_{0};
  mutable std::atomic<uint32_t> total_responses_{0};
  
  // Threads
  std::unique_ptr<std::thread> scheduler_thread_;
  std::unordered_map<uint32_t, std::unique_ptr<std::thread>> bot_threads_;
};

} // namespace hh::server

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bot_manager.hpp"

#include "hh/logging.hpp"

#include <thread>
#include <chrono>
#include <algorithm>

namespace hh::server {

// ── BotManager ──────────────────────────────────────────────────────

BotManager::BotManager(BotManager&& other) noexcept
  : bots_(std::move(other.bots_))
  , next_bot_id_(other.next_bot_id_)
  , commands_(std::move(other.commands_))
  , bot_response_callback_(std::move(other.bot_response_callback_))
  , total_commands_(other.total_commands_.load())
  , total_responses_(other.total_responses_.load())
  , scheduler_thread_(std::move(other.scheduler_thread_))
  , bot_threads_(std::move(other.bot_threads_)) {
  
  other.next_bot_id_ = 1;
  other.total_commands_ = 0;
  other.total_responses_ = 0;
}

BotManager& BotManager::operator=(BotManager&& other) noexcept {
  if (this != &other) {
    bots_ = std::move(other.bots_);
    next_bot_id_ = other.next_bot_id_;
    commands_ = std::move(other.commands_);
    bot_response_callback_ = std::move(other.bot_response_callback_);
    total_commands_ = other.total_commands_.load();
    total_responses_ = other.total_responses_.load();
    scheduler_thread_ = std::move(other.scheduler_thread_);
    bot_threads_ = std::move(other.bot_threads_);
    other.next_bot_id_ = 1;
    other.total_commands_ = 0;
    other.total_responses_ = 0;
  }
  return *this;
}

bool BotManager::add_bot(const Bot& bot) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (bots_.find(bot.bot_id) != bots_.end()) {
    hh::get_logger().warn("BotManager: Bot {} already exists", bot.bot_id);
    return false;
  }
  
  bots_[bot.bot_id] = bot;
  
  hh::get_logger().info("BotManager: Bot {} added", bot.bot_id);
  return true;
}

bool BotManager::remove_bot(uint32_t bot_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it == bots_.end()) {
    hh::get_logger().warn("BotManager: Bot {} not found", bot_id);
    return false;
  }
  
  bots_.erase(it);
  
  // Befehle entfernen
  commands_.erase(bot_id);
  
  // Thread stoppen
  auto thread_it = bot_threads_.find(bot_id);
  if (thread_it != bot_threads_.end()) {
    if (thread_it->second && thread_it->second->joinable()) {
      thread_it->second->join();
    }
    bot_threads_.erase(thread_it);
  }
  
  hh::get_logger().info("BotManager: Bot {} removed", bot_id);
  return true;
}

Bot* BotManager::get_bot(uint32_t bot_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it != bots_.end()) {
    return &it->second;
  }
  return nullptr;
}

const Bot* BotManager::get_bot(uint32_t bot_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it != bots_.end()) {
    return &it->second;
  }
  return nullptr;
}

Bot* BotManager::get_bot_by_name(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  for (auto& [id, bot] : bots_) {
    if (bot.name == name) {
      return &bot;
    }
  }
  return nullptr;
}

std::vector<Bot> BotManager::get_all_bots() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Bot> bots;
  for (const auto& [id, bot] : bots_) {
    bots.push_back(bot);
  }
  return bots;
}

std::vector<Bot> BotManager::get_active_bots() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::vector<Bot> bots;
  for (const auto& [id, bot] : bots_) {
    if (bot.is_active) {
      bots.push_back(bot);
    }
  }
  return bots;
}

void BotManager::activate_bot(uint32_t bot_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it != bots_.end()) {
    it->second.is_active = true;
    hh::get_logger().info("BotManager: Bot {} activated", bot_id);
  }
}

void BotManager::deactivate_bot(uint32_t bot_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it != bots_.end()) {
    it->second.is_active = false;
    hh::get_logger().info("BotManager: Bot {} deactivated", bot_id);
  }
}

void BotManager::update_bot_activity(uint32_t bot_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it != bots_.end()) {
    it->second.last_activity = std::chrono::duration<double>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
  }
}

bool BotManager::register_command(uint32_t bot_id, const std::string& command, CommandCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  if (bots_.find(bot_id) == bots_.end()) {
    hh::get_logger().warn("BotManager: Bot {} not found", bot_id);
    return false;
  }
  
  commands_[bot_id][command] = std::move(callback);
  
  // Bot-Befehle aktualisieren
  auto bot_it = bots_.find(bot_id);
  if (bot_it != bots_.end()) {
    bot_it->second.commands.push_back(command);
  }
  
  hh::get_logger().info("BotManager: Command '{}' registered for bot {}", command, bot_id);
  return true;
}

bool BotManager::unregister_command(uint32_t bot_id, const std::string& command) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto bot_commands_it = commands_.find(bot_id);
  if (bot_commands_it == commands_.end()) {
    return false;
  }
  
  auto cmd_it = bot_commands_it->second.find(command);
  if (cmd_it == bot_commands_it->second.end()) {
    return false;
  }
  
  bot_commands_it->second.erase(cmd_it);
  
  // Bot-Befehle aktualisieren
  auto bot_it = bots_.find(bot_id);
  if (bot_it != bots_.end()) {
    auto& cmds = bot_it->second.commands;
    cmds.erase(std::remove(cmds.begin(), cmds.end(), command), cmds.end());
  }
  
  hh::get_logger().info("BotManager: Command '{}' unregistered for bot {}", command, bot_id);
  return true;
}

bool BotManager::execute_command(uint32_t bot_id, const std::string& command, const std::string& args, uint32_t sender_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  total_commands_++;
  
  auto bot_commands_it = commands_.find(bot_id);
  if (bot_commands_it == commands_.end()) {
    hh::get_logger().warn("BotManager: No commands registered for bot {}", bot_id);
    return false;
  }
  
  auto cmd_it = bot_commands_it->second.find(command);
  if (cmd_it == bot_commands_it->second.end()) {
    hh::get_logger().warn("BotManager: Command '{}' not found for bot {}", command, bot_id);
    return false;
  }
  
  // Berechtigungen prüfen
  auto bot_it = bots_.find(bot_id);
  if (bot_it != bots_.end()) {
    auto perm_it = bot_it->second.permissions.find("can_execute_commands");
    if (perm_it != bot_it->second.permissions.end() && perm_it->second != "true") {
      hh::get_logger().warn("BotManager: Bot {} does not have permission to execute commands", bot_id);
      return false;
    }
  }
  
  // Befehl ausführen
  bool result = cmd_it->second(command, args, sender_id);
  
  if (result) {
    total_responses_++;
  }
  
  hh::get_logger().info("BotManager: Command '{}' executed for bot {}", command, bot_id);
  return result;
}

bool BotManager::set_bot_permission(uint32_t bot_id, const std::string& permission, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it == bots_.end()) {
    return false;
  }
  
  it->second.permissions[permission] = value;
  return true;
}

std::string BotManager::get_bot_permission(uint32_t bot_id, const std::string& permission) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it == bots_.end()) {
    return "";
  }
  
  auto perm_it = it->second.permissions.find(permission);
  if (perm_it != it->second.permissions.end()) {
    return perm_it->second;
  }
  
  return "";
}

bool BotManager::has_bot_permission(uint32_t bot_id, const std::string& permission) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it == bots_.end()) {
    return false;
  }
  
  auto perm_it = it->second.permissions.find(permission);
  if (perm_it != it->second.permissions.end()) {
    return perm_it->second == "true";
  }
  
  return false;
}

bool BotManager::set_bot_setting(uint32_t bot_id, const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it == bots_.end()) {
    return false;
  }
  
  it->second.settings[key] = value;
  return true;
}

std::string BotManager::get_bot_setting(uint32_t bot_id, const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = bots_.find(bot_id);
  if (it == bots_.end()) {
    return "";
  }
  
  auto setting_it = it->second.settings.find(key);
  if (setting_it != it->second.settings.end()) {
    return setting_it->second;
  }
  
  return "";
}

void BotManager::set_bot_response_callback(BotResponseCallback callback) {
  bot_response_callback_ = std::move(callback);
}

void BotManager::send_bot_response(uint32_t bot_id, const std::string& response) {
  if (bot_response_callback_) {
    bot_response_callback_(bot_id, response);
  }
}

BotManager::BotStats BotManager::get_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  BotStats stats;
  stats.total_bots = bots_.size();
  stats.active_bots = std::count_if(bots_.begin(), bots_.end(),
                                    [](const auto& pair) { return pair.second.is_active; });
  stats.inactive_bots = stats.total_bots - stats.active_bots;
  stats.total_commands = total_commands_.load();
  stats.total_responses = total_responses_.load();
  
  return stats;
}

void BotManager::reset_stats() {
  total_commands_ = 0;
  total_responses_ = 0;
}

void BotManager::bot_thread_func(uint32_t bot_id) {
  // Bot-Thread
  hh::get_logger().info("BotManager: Bot {} thread started", bot_id);
  
  // Bot-Logik
  
  hh::get_logger().info("BotManager: Bot {} thread stopped", bot_id);
}

void BotManager::scheduler_thread_func() {
  // Bot-Scheduler-Thread
  hh::get_logger().info("BotManager: Scheduler thread started");
  
  while (true) {
    // Aktive Bots verarbeiten
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [id, bot] : bots_) {
      if (bot.is_active) {
        // Bot-Aktivität aktualisieren
        bot.last_activity = std::chrono::duration<double>(
          std::chrono::system_clock::now().time_since_epoch()
        ).count();
      }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

} // namespace hh::server

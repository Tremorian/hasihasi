// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "performance_optimizer.hpp"

#include "hh/logging.hpp"

#include <thread>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace hh::server {

// ── PerformanceOptimizer ────────────────────────────────────────────

PerformanceOptimizer::PerformanceOptimizer(PerformanceOptimizer&& other) noexcept
  : cache_config_(other.cache_config_)
  , thread_pool_config_(other.thread_pool_config_)
  , connection_pool_config_(other.connection_pool_config_)
  , memory_pool_config_(other.memory_pool_config_)
  , cache_(std::move(other.cache_))
  , cache_hits_(other.cache_hits_)
  , cache_misses_(other.cache_misses_)
  , tasks_(std::move(other.tasks_))
  , thread_pool_threads_(std::move(other.thread_pool_threads_))
  , is_thread_pool_running_(other.is_thread_pool_running_.load())
  , tasks_submitted_(other.tasks_submitted_)
  , tasks_completed_(other.tasks_completed_)
  , tasks_failed_(other.tasks_failed_)
  , connections_(std::move(other.connections_))
  , next_connection_id_(other.next_connection_id_)
  , is_connection_pool_running_(other.is_connection_pool_running_.load())
  , connections_created_(other.connections_created_)
  , connections_closed_(other.connections_closed_)
  , memory_blocks_(std::move(other.memory_blocks_))
  , free_blocks_(std::move(other.free_blocks_))
  , allocated_memory_(other.allocated_memory_)
  , allocations_(other.allocations_)
  , deallocations_(other.deallocations_)
  , is_memory_pool_running_(other.is_memory_pool_running_.load())
  , cache_cleanup_thread_(std::move(other.cache_cleanup_thread_)) {
  
  other.cache_hits_ = 0;
  other.cache_misses_ = 0;
  other.is_thread_pool_running_ = false;
  other.tasks_submitted_ = 0;
  other.tasks_completed_ = 0;
  other.tasks_failed_ = 0;
  other.next_connection_id_ = 1;
  other.is_connection_pool_running_ = false;
  other.connections_created_ = 0;
  other.connections_closed_ = 0;
  other.allocated_memory_ = 0;
  other.allocations_ = 0;
  other.deallocations_ = 0;
  other.is_memory_pool_running_ = false;
}

PerformanceOptimizer& PerformanceOptimizer::operator=(PerformanceOptimizer&& other) noexcept {
  if (this != &other) {
    cache_config_ = other.cache_config_;
    thread_pool_config_ = other.thread_pool_config_;
    connection_pool_config_ = other.connection_pool_config_;
    memory_pool_config_ = other.memory_pool_config_;
    cache_ = std::move(other.cache_);
    cache_hits_ = other.cache_hits_;
    cache_misses_ = other.cache_misses_;
    tasks_ = std::move(other.tasks_);
    thread_pool_threads_ = std::move(other.thread_pool_threads_);
    is_thread_pool_running_ = other.is_thread_pool_running_.load();
    tasks_submitted_ = other.tasks_submitted_;
    tasks_completed_ = other.tasks_completed_;
    tasks_failed_ = other.tasks_failed_;
    connections_ = std::move(other.connections_);
    next_connection_id_ = other.next_connection_id_;
    is_connection_pool_running_ = other.is_connection_pool_running_.load();
    connections_created_ = other.connections_created_;
    connections_closed_ = other.connections_closed_;
    memory_blocks_ = std::move(other.memory_blocks_);
    free_blocks_ = std::move(other.free_blocks_);
    allocated_memory_ = other.allocated_memory_;
    allocations_ = other.allocations_;
    deallocations_ = other.deallocations_;
    is_memory_pool_running_ = other.is_memory_pool_running_.load();
    cache_cleanup_thread_ = std::move(other.cache_cleanup_thread_);
    other.cache_hits_ = 0;
    other.cache_misses_ = 0;
    other.is_thread_pool_running_ = false;
    other.tasks_submitted_ = 0;
    other.tasks_completed_ = 0;
    other.tasks_failed_ = 0;
    other.next_connection_id_ = 1;
    other.is_connection_pool_running_ = false;
    other.connections_created_ = 0;
    other.connections_closed_ = 0;
    other.allocated_memory_ = 0;
    other.allocations_ = 0;
    other.deallocations_ = 0;
    other.is_memory_pool_running_ = false;
  }
  return *this;
}

// ── Cache ───────────────────────────────────────────────────────────

void PerformanceOptimizer::set_cache_config(const CacheConfig& config) {
  cache_config_ = config;
}

PerformanceOptimizer::CacheConfig PerformanceOptimizer::get_cache_config() const {
  return cache_config_;
}

bool PerformanceOptimizer::cache_set(const std::string& key, const std::string& value, int ttl) {
  if (!cache_config_.enabled) {
    return false;
  }
  
  std::lock_guard<std::mutex> lock(cache_mutex_);
  
  double expires_at = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count() + (ttl > 0 ? ttl : cache_config_.ttl_seconds);
  
  cache_[key] = {value, expires_at};
  
  // Cache-Größe begrenzen
  while (cache_.size() > cache_config_.max_size) {
    auto it = cache_.begin();
    cache_.erase(it);
  }
  
  return true;
}

bool PerformanceOptimizer::cache_get(const std::string& key, std::string& value) {
  if (!cache_config_.enabled) {
    return false;
  }
  
  std::lock_guard<std::mutex> lock(cache_mutex_);
  
  auto it = cache_.find(key);
  if (it == cache_.end()) {
    cache_misses_++;
    return false;
  }
  
  // TTL prüfen
  double now = std::chrono::duration<double>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
  
  if (now > it->second.expires_at) {
    cache_.erase(it);
    cache_misses_++;
    return false;
  }
  
  value = it->second.value;
  cache_hits_++;
  return true;
}

bool PerformanceOptimizer::cache_delete(const std::string& key) {
  if (!cache_config_.enabled) {
    return false;
  }
  
  std::lock_guard<std::mutex> lock(cache_mutex_);
  
  auto it = cache_.find(key);
  if (it == cache_.end()) {
    return false;
  }
  
  cache_.erase(it);
  return true;
}

void PerformanceOptimizer::cache_clear() {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  
  cache_.clear();
}

size_t PerformanceOptimizer::cache_size() const {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  
  return cache_.size();
}

double PerformanceOptimizer::cache_hit_rate() const {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  
  size_t total = cache_hits_ + cache_misses_;
  if (total == 0) {
    return 0.0;
  }
  
  return static_cast<double>(cache_hits_) / static_cast<double>(total);
}

// ── Thread-Pool ─────────────────────────────────────────────────────

void PerformanceOptimizer::set_thread_pool_config(const ThreadPoolConfig& config) {
  thread_pool_config_ = config;
}

PerformanceOptimizer::ThreadPoolConfig PerformanceOptimizer::get_thread_pool_config() const {
  return thread_pool_config_;
}

bool PerformanceOptimizer::start_thread_pool() {
  if (is_thread_pool_running_) {
    hh::get_logger().warn("PerformanceOptimizer: Thread pool already running");
    return false;
  }
  
  is_thread_pool_running_ = true;
  
  for (int i = 0; i < thread_pool_config_.num_threads; i++) {
    thread_pool_threads_.push_back(std::make_unique<std::thread>([this]() {
      thread_pool_thread_func();
    }));
  }
  
  hh::get_logger().info("PerformanceOptimizer: Thread pool started with {} threads", thread_pool_config_.num_threads);
  return true;
}

void PerformanceOptimizer::stop_thread_pool() {
  if (!is_thread_pool_running_) {
    hh::get_logger().warn("PerformanceOptimizer: Thread pool not running");
    return;
  }
  
  is_thread_pool_running_ = false;
  condition_.notify_all();
  
  for (auto& thread : thread_pool_threads_) {
    if (thread && thread->joinable()) {
      thread->join();
    }
  }
  
  thread_pool_threads_.clear();
  
  hh::get_logger().info("PerformanceOptimizer: Thread pool stopped");
}

void PerformanceOptimizer::thread_pool_thread_func() {
  hh::get_logger().info("PerformanceOptimizer: Thread pool thread started");
  
  while (is_thread_pool_running_) {
    std::function<void()> task;
    
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait_for(lock, std::chrono::milliseconds(thread_pool_config_.task_timeout_ms), [this]() {
        return !tasks_.empty() || !is_thread_pool_running_;
      });
      
      if (!is_thread_pool_running_ || tasks_.empty()) {
        continue;
      }
      
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    
    try {
      task();
      tasks_completed_++;
    } catch (const std::exception& e) {
      hh::get_logger().error("PerformanceOptimizer: Task failed: {}", e.what());
      tasks_failed_++;
    }
  }
  
  hh::get_logger().info("PerformanceOptimizer: Thread pool thread stopped");
}

// ── Connection-Pool ─────────────────────────────────────────────────

void PerformanceOptimizer::set_connection_pool_config(const ConnectionPoolConfig& config) {
  connection_pool_config_ = config;
}

PerformanceOptimizer::ConnectionPoolConfig PerformanceOptimizer::get_connection_pool_config() const {
  return connection_pool_config_;
}

bool PerformanceOptimizer::start_connection_pool() {
  if (is_connection_pool_running_) {
    hh::get_logger().warn("PerformanceOptimizer: Connection pool already running");
    return false;
  }
  
  is_connection_pool_running_ = true;
  
  // Verbindungen vorbereiten
  for (size_t i = 0; i < connection_pool_config_.min_connections; i++) {
    Connection conn;
    conn.connection_id = next_connection_id_++;
    conn.is_available = true;
    conn.last_used = std::chrono::duration<double>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    connections_.push_back(conn);
    connections_created_++;
  }
  
  hh::get_logger().info("PerformanceOptimizer: Connection pool started with {} connections", connection_pool_config_.min_connections);
  return true;
}

void PerformanceOptimizer::stop_connection_pool() {
  if (!is_connection_pool_running_) {
    hh::get_logger().warn("PerformanceOptimizer: Connection pool not running");
    return;
  }
  
  is_connection_pool_running_ = false;
  
  connections_.clear();
  
  hh::get_logger().info("PerformanceOptimizer: Connection pool stopped");
}

PerformanceOptimizer::Connection* PerformanceOptimizer::get_connection() {
  std::lock_guard<std::mutex> lock(connection_mutex_);
  
  // Verfügbare Verbindung suchen
  for (auto& conn : connections_) {
    if (conn.is_available) {
      conn.is_available = false;
      conn.last_used = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count();
      conn.use_count++;
      return &conn;
    }
  }
  
  // Neue Verbindung erstellen, wenn noch nicht erreicht
  if (connections_.size() < connection_pool_config_.max_connections) {
    Connection conn;
    conn.connection_id = next_connection_id_++;
    conn.is_available = false;
    conn.last_used = std::chrono::duration<double>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    conn.use_count = 1;
    
    connections_.push_back(conn);
    connections_created_++;
    return &connections_.back();
  }
  
  return nullptr;
}

void PerformanceOptimizer::release_connection(uint32_t connection_id) {
  std::lock_guard<std::mutex> lock(connection_mutex_);
  
  for (auto& conn : connections_) {
    if (conn.connection_id == connection_id) {
      conn.is_available = true;
      return;
    }
  }
}

size_t PerformanceOptimizer::get_available_connections() const {
  std::lock_guard<std::mutex> lock(connection_mutex_);
  
  return std::count_if(connections_.begin(), connections_.end(),
                       [](const Connection& conn) { return conn.is_available; });
}

size_t PerformanceOptimizer::get_total_connections() const {
  std::lock_guard<std::mutex> lock(connection_mutex_);
  
  return connections_.size();
}

// ── Memory-Pool ─────────────────────────────────────────────────────

void PerformanceOptimizer::set_memory_pool_config(const MemoryPoolConfig& config) {
  memory_pool_config_ = config;
}

PerformanceOptimizer::MemoryPoolConfig PerformanceOptimizer::get_memory_pool_config() const {
  return memory_pool_config_;
}

bool PerformanceOptimizer::start_memory_pool() {
  if (is_memory_pool_running_) {
    hh::get_logger().warn("PerformanceOptimizer: Memory pool already running");
    return false;
  }
  
  is_memory_pool_running_ = true;
  
  // Speicherblöcke vorbereiten
  for (size_t i = 0; i < memory_pool_config_.num_blocks; i++) {
    auto block = std::make_unique<char[]>(memory_pool_config_.block_size);
    free_blocks_.push(block.get());
    memory_blocks_.push_back(std::move(block));
  }
  
  hh::get_logger().info("PerformanceOptimizer: Memory pool started with {} blocks", memory_pool_config_.num_blocks);
  return true;
}

void PerformanceOptimizer::stop_memory_pool() {
  if (!is_memory_pool_running_) {
    hh::get_logger().warn("PerformanceOptimizer: Memory pool not running");
    return;
  }
  
  is_memory_pool_running_ = false;
  
  memory_blocks_.clear();
  free_blocks_ = std::queue<char*>();
  
  hh::get_logger().info("PerformanceOptimizer: Memory pool stopped");
}

void* PerformanceOptimizer::allocate(size_t size) {
  if (!is_memory_pool_running_ || !memory_pool_config_.enabled) {
    return nullptr;
  }
  
  std::lock_guard<std::mutex> lock(memory_mutex_);
  
  if (free_blocks_.empty()) {
    return nullptr;
  }
  
  char* block = free_blocks_.front();
  free_blocks_.pop();
  
  allocated_memory_ += size;
  allocations_++;
  
  return static_cast<void*>(block);
}

void PerformanceOptimizer::deallocate(void* ptr) {
  if (!is_memory_pool_running_ || !memory_pool_config_.enabled) {
    return;
  }
  
  std::lock_guard<std::mutex> lock(memory_mutex_);
  
  if (ptr) {
    free_blocks_.push(static_cast<char*>(ptr));
    deallocations_++;
  }
}

size_t PerformanceOptimizer::get_allocated_memory() const {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  
  return allocated_memory_;
}

size_t PerformanceOptimizer::get_free_memory() const {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  
  return free_blocks_.size() * memory_pool_config_.block_size;
}

// ── Statistiken ─────────────────────────────────────────────────────

PerformanceOptimizer::PerformanceStats PerformanceOptimizer::get_stats() const {
  PerformanceStats stats;
  
  // Cache
  stats.cache_size = cache_size();
  stats.cache_hit_rate = cache_hit_rate();
  stats.cache_hits = cache_hits_;
  stats.cache_misses = cache_misses_;
  
  // Thread-Pool
  stats.tasks_submitted = tasks_submitted_;
  stats.tasks_completed = tasks_completed_;
  stats.tasks_failed = tasks_failed_;
  stats.queue_size = tasks_.size();
  
  // Connection-Pool
  stats.total_connections = get_total_connections();
  stats.available_connections = get_available_connections();
  stats.connections_created = connections_created_;
  stats.connections_closed = connections_closed_;
  
  // Memory-Pool
  stats.allocated_memory = get_allocated_memory();
  stats.free_memory = get_free_memory();
  stats.allocations = allocations_;
  stats.deallocations = deallocations_;
  
  return stats;
}

void PerformanceOptimizer::reset_stats() {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  cache_hits_ = 0;
  cache_misses_ = 0;
  
  tasks_submitted_ = 0;
  tasks_completed_ = 0;
  tasks_failed_ = 0;
  
  connections_created_ = 0;
  connections_closed_ = 0;
  
  allocated_memory_ = 0;
  allocations_ = 0;
  deallocations_ = 0;
}

} // namespace hh::server

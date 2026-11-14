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
#include <queue>
#include <thread>
#include <future>

namespace hh::server {

// Performance-Optimierungen für den HasiHasi-Server
class PerformanceOptimizer {
public:
  struct CacheConfig {
    size_t max_size = 1000;
    int ttl_seconds = 300;
    bool enabled = true;
  };

  struct ThreadPoolConfig {
    int num_threads = 4;
    size_t max_queue_size = 1000;
    int task_timeout_ms = 5000;
  };

  struct ConnectionPoolConfig {
    size_t min_connections = 5;
    size_t max_connections = 50;
    int connection_timeout_ms = 3000;
    int idle_timeout_seconds = 300;
  };

  PerformanceOptimizer() = default;
  ~PerformanceOptimizer() = default;

  // Nicht kopierbar, bewegbar
  PerformanceOptimizer(const PerformanceOptimizer&) = delete;
  PerformanceOptimizer& operator=(const PerformanceOptimizer&) = delete;
  PerformanceOptimizer(PerformanceOptimizer&&) noexcept;
  PerformanceOptimizer& operator=(PerformanceOptimizer&&) noexcept;

  // Cache-Verwaltung
  void set_cache_config(const CacheConfig& config);
  CacheConfig get_cache_config() const;
  
  bool cache_set(const std::string& key, const std::string& value, int ttl = -1);
  bool cache_get(const std::string& key, std::string& value);
  bool cache_delete(const std::string& key);
  void cache_clear();
  size_t cache_size() const;
  double cache_hit_rate() const;

  // Thread-Pool
  void set_thread_pool_config(const ThreadPoolConfig& config);
  ThreadPoolConfig get_thread_pool_config() const;
  
  bool start_thread_pool();
  void stop_thread_pool();
  bool is_thread_pool_running() const { return is_thread_pool_running_; }
  
  template<typename Func>
  std::future<typename std::invoke_result<Func>::type> submit_task(Func&& func) {
    auto task = std::make_shared<std::packaged_task<typename std::invoke_result<Func>::type()>>(
      std::forward<Func>(func)
    );
    
    std::future<typename std::invoke_result<Func>::type> result = task->get_future();
    
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (tasks_.size() >= config_.max_queue_size) {
        throw std::runtime_error("Thread pool queue is full");
      }
      tasks_.push([task]() { (*task)(); });
    }
    
    condition_.notify_one();
    return result;
  }

  // Connection Pool
  void set_connection_pool_config(const ConnectionPoolConfig& config);
  ConnectionPoolConfig get_connection_pool_config() const;
  
  bool start_connection_pool();
  void stop_connection_pool();
  bool is_connection_pool_running() const { return is_connection_pool_running_; }
  
  struct Connection {
    uint32_t connection_id;
    bool is_available = true;
    double last_used = 0.0;
    int use_count = 0;
  };
  
  Connection* get_connection();
  void release_connection(uint32_t connection_id);
  size_t get_available_connections() const;
  size_t get_total_connections() const;

  // Memory Pool
  struct MemoryPoolConfig {
    size_t block_size = 1024;
    size_t num_blocks = 1000;
    bool enabled = true;
  };
  
  void set_memory_pool_config(const MemoryPoolConfig& config);
  MemoryPoolConfig get_memory_pool_config() const;
  
  bool start_memory_pool();
  void stop_memory_pool();
  bool is_memory_pool_running() const { return is_memory_pool_running_; }
  
  void* allocate(size_t size);
  void deallocate(void* ptr);
  size_t get_allocated_memory() const;
  size_t get_free_memory() const;

  // Statistiken
  struct PerformanceStats {
    // Cache
    size_t cache_size = 0;
    double cache_hit_rate = 0.0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    // Thread Pool
    size_t tasks_submitted = 0;
    size_t tasks_completed = 0;
    size_t tasks_failed = 0;
    size_t queue_size = 0;
    
    // Connection Pool
    size_t total_connections = 0;
    size_t available_connections = 0;
    size_t connections_created = 0;
    size_t connections_closed = 0;
    
    // Memory Pool
    size_t allocated_memory = 0;
    size_t free_memory = 0;
    size_t allocations = 0;
    size_t deallocations = 0;
  };
  
  PerformanceStats get_stats() const;
  void reset_stats();

private:
  // Cache-Eintrag
  struct CacheEntry {
    std::string value;
    double expires_at;
  };
  
  // Cache-Thread
  void cache_cleanup_thread_func();
  
  // Thread-Pool-Thread
  void thread_pool_thread_func();
  
  // Connection-Pool-Thread
  void connection_pool_thread_func();
  
  // Einstellungen
  CacheConfig cache_config_;
  ThreadPoolConfig thread_pool_config_;
  ConnectionPoolConfig connection_pool_config_;
  MemoryPoolConfig memory_pool_config_;
  
  // Cache
  mutable std::mutex cache_mutex_;
  std::unordered_map<std::string, CacheEntry> cache_;
  size_t cache_hits_ = 0;
  size_t cache_misses_ = 0;
  
  // Thread-Pool
  mutable std::mutex mutex_;
  std::queue<std::function<void()>> tasks_;
  std::vector<std::unique_ptr<std::thread>> thread_pool_threads_;
  std::atomic<bool> is_thread_pool_running_ = false;
  std::condition_variable condition_;
  size_t tasks_submitted_ = 0;
  size_t tasks_completed_ = 0;
  size_t tasks_failed_ = 0;
  
  // Connection-Pool
  mutable std::mutex connection_mutex_;
  std::vector<Connection> connections_;
  uint32_t next_connection_id_ = 1;
  std::atomic<bool> is_connection_pool_running_ = false;
  size_t connections_created_ = 0;
  size_t connections_closed_ = 0;
  
  // Memory-Pool
  mutable std::mutex memory_mutex_;
  std::vector<std::unique_ptr<char[]>> memory_blocks_;
  std::queue<char*> free_blocks_;
  size_t allocated_memory_ = 0;
  size_t allocations_ = 0;
  size_t deallocations_ = 0;
  std::atomic<bool> is_memory_pool_running_ = false;
  
  // Cache-Cleanup-Thread
  std::unique_ptr<std::thread> cache_cleanup_thread_;
};

} // namespace hh::server

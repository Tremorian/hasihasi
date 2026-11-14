// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/rate_limiter.hpp"
#include "hh/logging.hpp"

#include <algorithm>

namespace hh::server {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct RateLimiter::Impl {
  ServerConfig config_;
  std::map<uint32_t, UserConfig> user_configs_;
  
  // Token-Bucket für Bandbreite
  struct TokenBucket {
    uint64_t max_tokens;
    uint64_t current_tokens;
    uint64_t refill_rate; // Tokens pro Sekunde
    std::chrono::steady_clock::time_point last_refill;

    TokenBucket()
      : max_tokens(0)
      , current_tokens(0)
      , refill_rate(0)
      , last_refill(std::chrono::steady_clock::now()) {
    }

    void initialize(uint64_t max_tokens, uint64_t refill_rate) {
      this->max_tokens = max_tokens;
      this->current_tokens = max_tokens;
      this->refill_rate = refill_rate;
      this->last_refill = std::chrono::steady_clock::now();
    }

    bool try_consume(uint64_t tokens) {
      // Refill
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill).count();
      uint64_t tokens_to_add = (elapsed * refill_rate) / 1000;
      current_tokens = std::min(max_tokens, current_tokens + tokens_to_add);
      last_refill = now;

      // Konsumieren
      if (current_tokens >= tokens) {
        current_tokens -= tokens;
        return true;
      }
      return false;
    }
  };

  // Packet Rate Counter
  struct PacketRateCounter {
    std::deque<std::chrono::steady_clock::time_point> timestamps_;
    uint64_t max_per_second;

    PacketRateCounter()
      : max_per_second(0) {
    }

    void initialize(uint64_t max_per_second) {
      this->max_per_second = max_per_second;
    }

    bool check() {
      auto now = std::chrono::steady_clock::now();
      
      // Alte Einträge entfernen (> 1 Sekunde)
      while (!timestamps_.empty() && 
             std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamps_.front()).count() > 1000) {
        timestamps_.pop_front();
      }

      // Prüfe Rate
      if (timestamps_.size() >= max_per_second) {
        return false;
      }

      timestamps_.push_back(now);
      return true;
    }

    void reset() {
      timestamps_.clear();
    }
  };

  // Flood-Erkennung
  struct FloodDetector {
    std::map<uint32_t, PacketRateCounter> counters_;
    std::map<uint32_t, std::chrono::steady_clock::time_point> ban_timers_;
    uint64_t flood_threshold;
    uint64_t ban_duration_ms;

    FloodDetector()
      : flood_threshold(0)
      , ban_duration_ms(0) {
    }

    void initialize(uint64_t flood_threshold, uint64_t ban_duration_ms) {
      this->flood_threshold = flood_threshold;
      this->ban_duration_ms = ban_duration_ms;
    }

    bool is_flooding(uint32_t user_id) {
      auto it = ban_timers_.find(user_id);
      if (it != ban_timers_.end()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
        if (elapsed < ban_duration_ms) {
          return true; // Noch im Bann
        } else {
          ban_timers_.erase(it); // Bann abgelaufen
        }
      }
      return false;
    }

    void check_and_ban(uint32_t user_id, uint64_t current_rate) {
      if (current_rate > flood_threshold) {
        ban_timers_[user_id] = std::chrono::steady_clock::now();
      }
    }

    void reset(uint32_t user_id) {
      auto it = ban_timers_.find(user_id);
      if (it != ban_timers_.end()) {
        ban_timers_.erase(it);
      }
    }

    bool is_banned(uint32_t user_id) const {
      auto it = ban_timers_.find(user_id);
      if (it != ban_timers_.end()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
        return elapsed < ban_duration_ms;
      }
      return false;
    }

    void unban(uint32_t user_id) {
      ban_timers_.erase(user_id);
    }
  };

  TokenBucket bandwidth_bucket_;
  PacketRateCounter packet_counter_;
  FloodDetector flood_detector_;
  
  // Statistik
  Stats stats_ = {};
};

// ── RateLimiter ───────────────────────────────────────────────────────

bool RateLimiter::initialize(const ServerConfig& config) {
  impl_ = std::make_unique<Impl>();
  config_ = config;

  impl_->config_ = config;
  impl_->bandwidth_bucket_.initialize(config.default_bandwidth_bps / 8, config.default_bandwidth_bps / 8);
  impl_->packet_counter_.initialize(config.default_packets_per_second);
  impl_->flood_detector_.initialize(config.flood_threshold, config.flood_ban_duration_ms);

  hh::get_logger().info("RateLimiter: Initialized (default bw: {} kbit/s, pps: {})",
                        config.default_bandwidth_bps / 1000,
                        config.default_packets_per_second);
  return true;
}

void RateLimiter::shutdown() {
  impl_.reset();
}

void RateLimiter::set_user_config(uint32_t user_id, const UserConfig& config) {
  if (!impl_) return;

  impl_->user_configs_[user_id] = config;
  impl_->bandwidth_bucket_.initialize(config.max_bandwidth_bps / 8, config.max_bandwidth_bps / 8);
  impl_->packet_counter_.initialize(config.max_packets_per_second);

  hh::get_logger().debug("RateLimiter: User {} config set (bw: {} kbit/s, pps: {})",
                        user_id, config.max_bandwidth_bps / 1000, config.max_packets_per_second);
}

RateLimiter::UserConfig RateLimiter::get_user_config(uint32_t user_id) const {
  if (!impl_) return {};

  auto it = impl_->user_configs_.find(user_id);
  if (it != impl_->user_configs_.end()) {
    return it->second;
  }
  return {};
}

bool RateLimiter::check_bandwidth(uint32_t user_id, size_t bytes) {
  if (!impl_ || !config_.enabled) {
    return true;
  }

  // Prüfe Bann
  if (impl_->flood_detector_.is_banned(user_id)) {
    stats_.bandwidth_dropped++;
    return false;
  }

  // Token-Bucket prüfen
  bool allowed = impl_->bandwidth_bucket_.try_consume(bytes);
  if (!allowed) {
    stats_.bandwidth_dropped++;
    hh::get_logger().debug("RateLimiter: User {} bandwidth exceeded ({} bytes)", user_id, bytes);
  } else {
    stats_.packets_allowed++;
  }

  return allowed;
}

bool RateLimiter::check_packet_rate(uint32_t user_id) {
  if (!impl_ || !config_.enabled) {
    return true;
  }

  // Prüfe Bann
  if (impl_->flood_detector_.is_banned(user_id)) {
    stats_.packets_dropped++;
    return false;
  }

  // Packet Rate prüfen
  bool allowed = impl_->packet_counter_.check();
  if (!allowed) {
    stats_.packets_dropped++;
    hh::get_logger().debug("RateLimiter: User {} packet rate exceeded", user_id);
  } else {
    stats_.packets_allowed++;
  }

  return allowed;
}

bool RateLimiter::is_flooding(uint32_t user_id) {
  if (!impl_) return false;
  return impl_->flood_detector_.is_flooding(user_id);
}

void RateLimiter::reset_flood_counter(uint32_t user_id) {
  if (!impl_) return;
  impl_->flood_detector_.reset(user_id);
  impl_->packet_counter_.reset();
}

bool RateLimiter::ban_for_flood(uint32_t user_id) {
  if (!impl_) return false;

  impl_->flood_detector_.check_and_ban(user_id, config_.flood_threshold);
  stats_.floods_detected++;
  stats_.bans_issued++;

  hh::get_logger().warn("RateLimiter: User {} banned for flooding", user_id);

  if (flood_callback_) {
    flood_callback_(user_id, "");
  }

  return true;
}

bool RateLimiter::is_banned(uint32_t user_id) const {
  if (!impl_) return false;
  return impl_->flood_detector_.is_banned(user_id);
}

void RateLimiter::unban_user(uint32_t user_id) {
  if (!impl_) return;
  impl_->flood_detector_.unban(user_id);
  hh::get_logger().info("RateLimiter: User {} unbanned", user_id);
}

RateLimiter::Stats RateLimiter::get_stats() const {
  if (!impl_) return {};
  return impl_->stats_;
}

void RateLimiter::reset_stats() {
  if (impl_) {
    impl_->stats_ = {};
  }
}

void RateLimiter::set_flood_callback(FloodCallback callback) {
  flood_callback_ = callback;
}

} // namespace hh::server

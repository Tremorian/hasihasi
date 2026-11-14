// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/image_manager.hpp"

#include "hh/logging.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

namespace hh::client {

// ── ImageManager ──────────────────────────────────────────────────────

ImageManager::ImageManager(ImageManager&& other) noexcept
  : block_remote_images_(other.block_remote_images_)
  , show_image_previews_(other.show_image_previews_)
  , max_image_size_mb_(other.max_image_size_mb_)
  , blocked_urls_(std::move(other.blocked_urls_))
  , allowed_urls_(std::move(other.allowed_urls_))
  , image_blocked_callback_(std::move(other.image_blocked_callback_))
  , image_allowed_callback_(std::move(other.image_allowed_callback_))
  , total_images_(other.total_images_.load())
  , blocked_images_(other.blocked_images_.load())
  , allowed_images_(other.allowed_images_.load())
  , downloaded_images_(other.downloaded_images_.load()) {
  
  other.block_remote_images_ = true;
  other.show_image_previews_ = false;
  other.max_image_size_mb_ = 5.0f;
}

ImageManager& ImageManager::operator=(ImageManager&& other) noexcept {
  if (this != &other) {
    block_remote_images_ = other.block_remote_images_;
    show_image_previews_ = other.show_image_previews_;
    max_image_size_mb_ = other.max_image_size_mb_;
    blocked_urls_ = std::move(other.blocked_urls_);
    allowed_urls_ = std::move(other.allowed_urls_);
    image_blocked_callback_ = std::move(other.image_blocked_callback_);
    image_allowed_callback_ = std::move(other.image_allowed_callback_);
    total_images_ = other.total_images_.load();
    blocked_images_ = other.blocked_images_.load();
    allowed_images_ = other.allowed_images_.load();
    downloaded_images_ = other.downloaded_images_.load();
    
    other.block_remote_images_ = true;
    other.show_image_previews_ = false;
    other.max_image_size_mb_ = 5.0f;
  }
  return *this;
}

bool ImageManager::process_image_url(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  total_images_++;
  
  // URL validieren
  if (!is_valid_url(url)) {
    hh::get_logger().warn("ImageManager: Invalid URL: {}", url);
    return false;
  }
  
  // Remote-Bilder blockieren
  if (block_remote_images_ && !is_allowed(url)) {
    blocked_images_++;
    
    if (image_blocked_callback_) {
      image_blocked_callback_(url);
    }
    
    hh::get_logger().info("ImageManager: Blocked remote image: {}", url);
    return false;
  }
  
  // Bild erlauben
  allowed_images_++;
  
  if (image_allowed_callback_) {
    image_allowed_callback_(url);
  }
  
  hh::get_logger().info("ImageManager: Allowed image: {}", url);
  return true;
}

void ImageManager::allow_image_url(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  allowed_urls_.push_back(url);
  
  hh::get_logger().info("ImageManager: Allowed URL: {}", url);
}

void ImageManager::block_image_url(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  blocked_urls_.push_back(url);
  
  hh::get_logger().info("ImageManager: Blocked URL: {}", url);
}

void ImageManager::clear_blocked_urls() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  blocked_urls_.clear();
  
  hh::get_logger().info("ImageManager: Cleared blocked URLs");
}

void ImageManager::clear_allowed_urls() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  allowed_urls_.clear();
  
  hh::get_logger().info("ImageManager: Cleared allowed URLs");
}

void ImageManager::set_block_remote_images(bool block) {
  block_remote_images_ = block;
  hh::get_logger().info("ImageManager: Block remote images: {}", block ? "enabled" : "disabled");
}

bool ImageManager::is_block_remote_images_enabled() const {
  return block_remote_images_;
}

void ImageManager::set_show_image_previews(bool show) {
  show_image_previews_ = show;
  hh::get_logger().info("ImageManager: Show image previews: {}", show ? "enabled" : "disabled");
}

bool ImageManager::is_show_image_previews_enabled() const {
  return show_image_previews_;
}

void ImageManager::set_max_image_size_mb(float max_size_mb) {
  max_image_size_mb_ = max_size_mb;
  hh::get_logger().info("ImageManager: Max image size: {} MB", max_size_mb);
}

float ImageManager::get_max_image_size_mb() const {
  return max_image_size_mb_;
}

void ImageManager::set_image_blocked_callback(ImageBlockedCallback callback) {
  image_blocked_callback_ = std::move(callback);
}

void ImageManager::set_image_allowed_callback(ImageAllowedCallback callback) {
  image_allowed_callback_ = std::move(callback);
}

ImageManager::ImageStats ImageManager::get_stats() const {
  ImageStats stats;
  stats.total_images = total_images_.load();
  stats.blocked_images = blocked_images_.load();
  stats.allowed_images = allowed_images_.load();
  stats.downloaded_images = downloaded_images_.load();
  return stats;
}

void ImageManager::reset_stats() {
  total_images_ = 0;
  blocked_images_ = 0;
  allowed_images_ = 0;
  downloaded_images_ = 0;
}

bool ImageManager::is_valid_url(const std::string& url) const {
  // Einfache URL-Validierung
  static const std::regex url_regex(R"(^https?://[^\s/$.?#].[^\s]*)");
  return std::regex_match(url, url_regex);
}

std::string ImageManager::extract_domain(const std::string& url) const {
  // Domain aus URL extrahieren
  size_t start = url.find("://");
  if (start == std::string::npos) {
    return url;
  }
  
  start += 3;
  size_t end = url.find('/', start);
  if (end == std::string::npos) {
    end = url.size();
  }
  
  return url.substr(start, end - start);
}

std::string ImageManager::detect_file_type(const std::string& url) const {
  // Dateierkennung basierend auf URL
  if (url.find(".jpg") != std::string::npos || url.find(".jpeg") != std::string::npos) {
    return "image/jpeg";
  } else if (url.find(".png") != std::string::npos) {
    return "image/png";
  } else if (url.find(".gif") != std::string::npos) {
    return "image/gif";
  } else if (url.find(".webp") != std::string::npos) {
    return "image/webp";
  } else if (url.find(".svg") != std::string::npos) {
    return "image/svg+xml";
  }
  
  return "application/octet-stream";
}

bool ImageManager::is_blocked(const std::string& url) const {
  return std::find(blocked_urls_.begin(), blocked_urls_.end(), url) != blocked_urls_.end();
}

bool ImageManager::is_allowed(const std::string& url) const {
  return std::find(allowed_urls_.begin(), allowed_urls_.end(), url) != allowed_urls_.end();
}

} // namespace hh::client

// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

namespace hh::client {

// Bilderverwaltung für Textchat
// Blockiert Remote-Bilder, es sei denn, der Benutzer stimmt explizit zu
class ImageManager {
public:
  ImageManager() = default;
  ~ImageManager() = default;

  // Nicht kopierbar, bewegbar
  ImageManager(const ImageManager&) = delete;
  ImageManager& operator=(const ImageManager&) = delete;
  ImageManager(ImageManager&&) noexcept;
  ImageManager& operator=(ImageManager&&) noexcept;

  // Bild-URL verarbeiten
  // Gibt true zurück, wenn das Bild angezeigt werden darf
  bool process_image_url(const std::string& url);
  
  // Bild-URL erlauben (vom Benutzer freigegeben)
  void allow_image_url(const std::string& url);
  
  // Bild-URL blockieren (vom Benutzer blockiert)
  void block_image_url(const std::string& url);
  
  // Alle blockierten URLs entfernen
  void clear_blocked_urls();
  
  // Alle erlaubten URLs entfernen
  void clear_allowed_urls();
  
  // Einstellungen
  void set_block_remote_images(bool block);
  bool is_block_remote_images_enabled() const { return block_remote_images_; }
  
  void set_show_image_previews(bool show);
  bool is_show_image_previews_enabled() const { return show_image_previews_; }
  
  void set_max_image_size_mb(float max_size_mb);
  float get_max_image_size_mb() const { return max_image_size_mb_; }
  
  // Callbacks
  using ImageBlockedCallback = std::function<void(const std::string& url)>;
  void set_image_blocked_callback(ImageBlockedCallback callback);
  
  using ImageAllowedCallback = std::function<void(const std::string& url)>;
  void set_image_allowed_callback(ImageAllowedCallback callback);
  
  // Statistiken
  struct ImageStats {
    uint32_t total_images = 0;
    uint32_t blocked_images = 0;
    uint32_t allowed_images = 0;
    uint32_t downloaded_images = 0;
  };
  
  ImageStats get_stats() const;
  void reset_stats();

private:
  // URL validieren
  bool is_valid_url(const std::string& url) const;
  
  // Domain extrahieren
  std::string extract_domain(const std::string& url) const;
  
  // Dateierkennung basierend auf URL
  std::string detect_file_type(const std::string& url) const;
  
  // Block-Liste prüfen
  bool is_blocked(const std::string& url) const;
  
  // Erlaubnis-Liste prüfen
  bool is_allowed(const std::string& url) const;
  
  // Einstellungen
  bool block_remote_images_ = true;
  bool show_image_previews_ = false;
  float max_image_size_mb_ = 5.0f;
  
  // Block-/Erlaubnis-Listen
  mutable std::mutex mutex_;
  std::vector<std::string> blocked_urls_;
  std::vector<std::string> allowed_urls_;
  
  // Callbacks
  ImageBlockedCallback image_blocked_callback_;
  ImageAllowedCallback image_allowed_callback_;
  
  // Statistiken
  mutable std::atomic<uint32_t> total_images_{0};
  mutable std::atomic<uint32_t> blocked_images_{0};
  mutable std::atomic<uint32_t> allowed_images_{0};
  mutable std::atomic<uint32_t> downloaded_images_{0};
};

} // namespace hh::client

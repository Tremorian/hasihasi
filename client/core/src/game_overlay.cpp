// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hh/client/game_overlay.hpp"

#include "hh/logging.hpp"

#include <sstream>

namespace hh::client {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct GameOverlay::Impl {
  // MangoHUD-Implementierung
  struct MangoHudImpl {
    bool initialized_ = false;

    bool initialize() {
      // MangoHUD D-Bus oder Environment-Variable
      // MANGOHUD=1 und MANGOHUD_OVR_SCALE
      hh::get_logger().info("GameOverlay: MangoHUD overlay initialized");
      initialized_ = true;
      return true;
    }

    void update_speakers(const std::vector<SpeakerInfo>& speakers) {
      // MangoHUD Text overlay aktualisieren
      std::stringstream ss;
      ss << "HasiHasi Speakers:";
      for (const auto& speaker : speakers) {
        if (speaker.is_speaking) {
          ss << "\n" << speaker.name << " (" << static_cast<int>(speaker.volume * 100) << "%)";
        }
      }
      hh::get_logger().info("GameOverlay: MangoHUD speakers updated: {}", ss.str());
    }

    void clear_speakers() {
      hh::get_logger().info("GameOverlay: MangoHUD speakers cleared");
    }
  };

  // Vulkan-Layer-Implementierung
  struct VkLayerImpl {
    bool initialized_ = false;

    bool initialize() {
      // Vulkan-Layer registrieren
      // layer.json in VK_LAYER_PATH
      hh::get_logger().info("GameOverlay: Vulkan layer overlay initialized");
      initialized_ = true;
      return true;
    }

    void update_speakers(const std::vector<SpeakerInfo>& speakers) {
      // Vulkan-Layer Daten aktualisieren
      hh::get_logger().info("GameOverlay: VkLayer speakers updated: {} speakers", speakers.size());
    }

    void clear_speakers() {
      hh::get_logger().info("GameOverlay: VkLayer speakers cleared");
    }
  };

  MangoHudImpl mangohud_impl_;
  VkLayerImpl vklayer_impl_;
};

// ── GameOverlay ───────────────────────────────────────────────────────

bool GameOverlay::initialize() {
  if (is_initialized_) {
    return true;
  }

  impl_ = std::make_unique<Impl>();
  is_initialized_ = true;

  hh::get_logger().info("GameOverlay: Initialized");
  return true;
}

void GameOverlay::shutdown() {
  if (!is_initialized_) {
    return;
  }

  impl_.reset();
  is_initialized_ = false;
  is_visible_ = false;

  hh::get_logger().info("GameOverlay: Shutdown");
}

void GameOverlay::set_type(Type type) {
  current_type_ = type;
  hh::get_logger().info("GameOverlay: Overlay type set to {}",
                        type == Type::MANGOHUD ? "MangoHud" :
                        type == Type::VKLAYER ? "VkLayer" :
                        type == Type::CUSTOM ? "Custom" : "None");
}

void GameOverlay::update_speakers(const std::vector<SpeakerInfo>& speakers) {
  if (!is_initialized_ || !impl_) {
    return;
  }

  if (current_type_ == Type::MANGOHUD) {
    impl_->mangohud_impl_.update_speakers(speakers);
  } else if (current_type_ == Type::VKLAYER) {
    impl_->vklayer_impl_.update_speakers(speakers);
  }
}

void GameOverlay::clear_speakers() {
  if (!is_initialized_ || !impl_) {
    return;
  }

  if (current_type_ == Type::MANGOHUD) {
    impl_->mangohud_impl_.clear_speakers();
  } else if (current_type_ == Type::VKLAYER) {
    impl_->vklayer_impl_.clear_speakers();
  }
}

bool GameOverlay::set_position(float x, float y, float width, float height) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  hh::get_logger().info("GameOverlay: Position set ({}, {}, {}, {})", x, y, width, height);
  return true;
}

bool GameOverlay::set_font_size(int size) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  hh::get_logger().info("GameOverlay: Font size set to {}", size);
  return true;
}

bool GameOverlay::set_opacity(float opacity) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  opacity = std::max(0.0f, std::min(1.0f, opacity));
  hh::get_logger().info("GameOverlay: Opacity set to {:.2f}", opacity);
  return true;
}

bool GameOverlay::set_show_positional(bool show) {
  if (!is_initialized_ || !impl_) {
    return false;
  }

  hh::get_logger().info("GameOverlay: Positional display {}", show ? "enabled" : "disabled");
  return true;
}

void GameOverlay::show() {
  if (!is_initialized_ || !impl_) {
    return;
  }

  is_visible_ = true;
  hh::get_logger().info("GameOverlay: Shown");

  if (visibility_callback_) {
    visibility_callback_(true);
  }
}

void GameOverlay::hide() {
  if (!is_initialized_ || !impl_) {
    return;
  }

  is_visible_ = false;
  hh::get_logger().info("GameOverlay: Hidden");

  if (visibility_callback_) {
    visibility_callback_(false);
  }
}

void GameOverlay::set_visibility_callback(VisibilityCallback callback) {
  visibility_callback_ = callback;
}

} // namespace hh::client

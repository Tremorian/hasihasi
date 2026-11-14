// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace hh::client {

// Overlay für Spiele – zeigt wer spricht
class GameOverlay {
public:
  GameOverlay() = default;
  ~GameOverlay() = default;

  // Initialisierung
  bool initialize();
  void shutdown();
  bool is_initialized() const { return is_initialized_; }

  // Overlay-Typ
  enum class Type {
    NONE,
    MANGOHUD,
    VKLAYER,
    CUSTOM
  };

  Type current_type() const { return current_type_; }
  void set_type(Type type);

  // Sprecher-Anzeige
  struct SpeakerInfo {
    std::string name;
    float volume; // 0.0 - 1.0
    bool is_speaking;
    float position_x; // -1.0 - 1.0 (Positional Audio)
    float position_y;
  };

  void update_speakers(const std::vector<SpeakerInfo>& speakers);
  void clear_speakers();

  // Konfiguration
  bool set_position(float x, float y, float width, float height);
  bool set_font_size(int size);
  bool set_opacity(float opacity); // 0.0 - 1.0
  bool set_show_positional(bool show);

  // Sichtbarkeit
  void show();
  void hide();
  bool is_visible() const { return is_visible_; }

  // Callbacks
  using VisibilityCallback = std::function<void(bool)>;
  void set_visibility_callback(VisibilityCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  bool is_initialized_ = false;
  bool is_visible_ = false;
  Type current_type_ = Type::NONE;
  VisibilityCallback visibility_callback_;
};

} // namespace hh::client

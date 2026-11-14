// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <array>
#include <functional>

namespace hh::server {

// AEAD-Verschlüsselung für Voice-Pakete mit AES-256-GCM
class VoiceCrypto {
public:
  // Schlüssel-Größe (AES-256)
  static constexpr size_t KEY_SIZE = 32;
  // Nonce-Größe (GCM: 12 Byte recommended)
  static constexpr size_t NONCE_SIZE = 12;
  // Auth-Tag-Größe (GCM: 16 Byte recommended)
  static constexpr size_t TAG_SIZE = 16;

  // Voice-Paket-Struktur: [header 1B][session id varint][sequence varint][target 1B][opus frame][auth tag]
  // Max Opus-Paketgröße: 1276 Byte (MTU 1500 - IP/UDP-Header)
  static constexpr size_t MAX_OPUS_SIZE = 1276;
  static constexpr size_t MAX_PACKET_SIZE = 1 + 16 + 16 + 1 + MAX_OPUS_SIZE + TAG_SIZE;

  struct Config {
    uint16_t rotation_interval_ms = 60000; // Schlüssel-Rotation alle 60 Sekunden
    size_t max_packets_per_key = 1000000;  // Oder nach 1M Paketen rotieren
    bool enabled = true;
  };

  VoiceCrypto() = default;
  ~VoiceCrypto() = default;

  // Initialisierung mit Schlüssel
  bool initialize(const std::array<uint8_t, KEY_SIZE>& key);

  // Verschlüsselung
  // input: rohes Opus-Paket
  // sequence: Sequenznummer (wird als Nonce verwendet)
  // output: verschlüsseltes Paket [session_id][sequence][target][opus][tag]
  struct EncryptResult {
    bool success;
    size_t output_size;
    uint64_t sequence;
  };

  EncryptResult encrypt(std::span<const uint8_t> input, uint64_t sequence,
                        uint8_t session_id, uint8_t target);

  // Verschlüsselung mit explizitem Nonce
  EncryptResult encrypt_with_nonce(std::span<const uint8_t> input,
                                   const std::array<uint8_t, NONCE_SIZE>& nonce,
                                   uint8_t session_id, uint8_t target);

  // Entschlüsselung
  // input: verschlüsseltes Paket
  // output: entschlüsseltes Opus-Paket
  struct DecryptResult {
    bool success;
    std::string error;
    std::vector<uint8_t> plaintext;
    uint64_t sequence;
    uint8_t session_id;
    uint8_t target;
  };

  DecryptResult decrypt(std::span<const uint8_t> input);

  // Schlüssel-Rotation
  void rotate_key();
  bool is_key_rotated() const { return key_rotated_; }

  // Konfiguration
  void set_config(const Config& config);
  Config config() const { return config_; }

  // Statistik
  struct Stats {
    size_t packets_encrypted;
    size_t packets_decrypted;
    size_t decryption_failures;
    size_t key_rotations;
    uint64_t current_sequence;
  };

  Stats get_stats() const;
  void reset_stats();

  // Callbacks
  using KeyRotationCallback = std::function<void(const std::array<uint8_t, KEY_SIZE>& new_key)>;
  void set_key_rotation_callback(KeyRotationCallback callback);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  Config config_;
  bool key_rotated_ = false;

  Stats stats_;
  KeyRotationCallback key_rotation_callback_;
};

} // namespace hh::server

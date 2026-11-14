// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/voice_crypto.hpp"

#include "hh/logging.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>

namespace hh::server {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct VoiceCrypto::Impl {
  std::array<uint8_t, KEY_SIZE> key_{};
  bool initialized_ = false;

  Impl() = default;
  ~Impl() = default;

  bool initialize(const std::array<uint8_t, KEY_SIZE>& key) {
    key_ = key;
    initialized_ = true;

    // Schlüssel als Hex kodieren für Logs (nur erste 16 Bytes)
    std::string key_hex;
    for (int i = 0; i < 16; ++i) {
      char buf[3];
      snprintf(buf, sizeof(buf), "%02x", key_[i]);
      key_hex += buf;
    }

    hh::get_logger().info("VoiceCrypto: Initialized with key {}", key_hex + "...");
    return true;
  }

  VoiceCrypto::EncryptResult encrypt(std::span<const uint8_t> input, uint64_t sequence,
                                      uint8_t session_id, uint8_t target) {
    EncryptResult result{};

    if (!initialized_) {
      hh::get_logger().error("VoiceCrypto: Not initialized");
      return result;
    }

    if (input.size() > VoiceCrypto::MAX_OPUS_SIZE) {
      hh::get_logger().error("VoiceCrypto: Input too large: {} bytes", input.size());
      return result;
    }

    // Paket aufbauen: [session_id][sequence][target][opus][tag]
    std::vector<uint8_t> output(VoiceCrypto::MAX_PACKET_SIZE);
    size_t offset = 0;

    // Session ID (1 Byte)
    output[offset++] = session_id;

    // Sequence (8 Byte, Big-Endian)
    for (int i = 7; i >= 0; --i) {
      output[offset++] = static_cast<uint8_t>((sequence >> (i * 8)) & 0xFF);
    }

    // Target (1 Byte)
    output[offset++] = target;

    // Opus Frame
    std::memcpy(output.data() + offset, input.data(), input.size());
    offset += input.size();

    // AEAD-Verschlüsselung mit AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
      hh::get_logger().error("VoiceCrypto: Failed to create EVP context");
      return result;
    }

    // Nonce aus Sequenznummer (12 Byte)
    std::array<uint8_t, VoiceCrypto::NONCE_SIZE> nonce{};
    for (int i = 0; i < 12 && i < 8; ++i) {
      nonce[i] = static_cast<uint8_t>((sequence >> (i * 8)) & 0xFF);
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptInit_ex failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptInit_ex key/nonce failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Input verschlüsseln
    int len = 0;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, output.data() + offset, &len, input.data(), input.size()) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptUpdate failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }
    ciphertext_len = len;

    // Final
    if (EVP_EncryptFinal_ex(ctx, output.data() + offset + ciphertext_len, &len) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptFinal_ex failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }
    ciphertext_len += len;

    // Tag holen
    std::array<uint8_t, VoiceCrypto::TAG_SIZE> tag{};
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, VoiceCrypto::TAG_SIZE, tag.data()) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_CTRL_GCM_GET_TAG failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Tag anhängen
    std::memcpy(output.data() + offset + ciphertext_len, tag.data(), VoiceCrypto::TAG_SIZE);
    offset += ciphertext_len + VoiceCrypto::TAG_SIZE;

    EVP_CIPHER_CTX_free(ctx);

    result.success = true;
    result.output_size = offset;
    result.sequence = sequence;

    return result;
  }

  VoiceCrypto::EncryptResult encrypt_with_nonce(std::span<const uint8_t> input,
                                                 const std::array<uint8_t, NONCE_SIZE>& nonce,
                                                 uint8_t session_id, uint8_t target) {
    EncryptResult result{};

    if (!initialized_) {
      hh::get_logger().error("VoiceCrypto: Not initialized");
      return result;
    }

    if (input.size() > VoiceCrypto::MAX_OPUS_SIZE) {
      hh::get_logger().error("VoiceCrypto: Input too large: {} bytes", input.size());
      return result;
    }

    // Paket aufbauen: [session_id][sequence][target][opus][tag]
    std::vector<uint8_t> output(VoiceCrypto::MAX_PACKET_SIZE);
    size_t offset = 0;

    // Session ID (1 Byte)
    output[offset++] = session_id;

    // Nonce wird nicht im Paket gespeichert, da deterministisch aus Sequenz generiert

    // Target (1 Byte)
    output[offset++] = target;

    // Opus Frame
    std::memcpy(output.data() + offset, input.data(), input.size());
    offset += input.size();

    // AEAD-Verschlüsselung mit AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
      hh::get_logger().error("VoiceCrypto: Failed to create EVP context");
      return result;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptInit_ex failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptInit_ex key/nonce failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Input verschlüsseln
    int len = 0;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, output.data() + offset, &len, input.data(), input.size()) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptUpdate failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }
    ciphertext_len = len;

    // Final
    if (EVP_EncryptFinal_ex(ctx, output.data() + offset + ciphertext_len, &len) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_EncryptFinal_ex failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }
    ciphertext_len += len;

    // Tag holen
    std::array<uint8_t, VoiceCrypto::TAG_SIZE> tag{};
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, VoiceCrypto::TAG_SIZE, tag.data()) != 1) {
      hh::get_logger().error("VoiceCrypto: EVP_CTRL_GCM_GET_TAG failed");
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Tag anhängen
    std::memcpy(output.data() + offset + ciphertext_len, tag.data(), VoiceCrypto::TAG_SIZE);
    offset += ciphertext_len + VoiceCrypto::TAG_SIZE;

    EVP_CIPHER_CTX_free(ctx);

    result.success = true;
    result.output_size = offset;
    result.sequence = 0; // Wird nicht verwendet bei explizitem Nonce

    return result;
  }

  VoiceCrypto::DecryptResult decrypt(std::span<const uint8_t> input) {
    DecryptResult result{};

    if (!initialized_) {
      result.error = "Not initialized";
      return result;
    }

    if (input.size() < VoiceCrypto::NONCE_SIZE + VoiceCrypto::TAG_SIZE + 3) {
      result.error = "Packet too small";
      return result;
    }

    // Paket parsen: [session_id][sequence][target][encrypted_opus][tag]
    size_t offset = 0;

    // Session ID
    uint8_t session_id = input[offset++];

    // Sequence (8 Byte, Big-Endian)
    uint64_t sequence = 0;
    for (int i = 0; i < 8; ++i) {
      sequence = (sequence << 8) | input[offset++];
    }

    // Target
    uint8_t target = input[offset++];

    // Encrypted Opus + Tag
    size_t encrypted_size = input.size() - offset;
    if (encrypted_size < VoiceCrypto::TAG_SIZE) {
      result.error = "No tag in packet";
      return result;
    }

    size_t ciphertext_size = encrypted_size - VoiceCrypto::TAG_SIZE;

    // Nonce aus Sequenznummer
    std::array<uint8_t, VoiceCrypto::NONCE_SIZE> nonce{};
    for (int i = 0; i < 12 && i < 8; ++i) {
      nonce[i] = static_cast<uint8_t>((sequence >> (i * 8)) & 0xFF);
    }

    // AEAD-Entschlüsselung mit AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
      result.error = "Failed to create EVP context";
      return result;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
      result.error = "EVP_DecryptInit_ex failed";
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()) != 1) {
      result.error = "EVP_DecryptInit_ex key/nonce failed";
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Input entschlüsseln
    int len = 0;
    int plaintext_len = 0;
    if (EVP_DecryptUpdate(ctx, nullptr, &len, input.data() + offset, ciphertext_size) != 1) {
      result.error = "EVP_DecryptUpdate failed";
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Tag setzen
    std::array<uint8_t, VoiceCrypto::TAG_SIZE> tag{};
    std::memcpy(tag.data(), input.data() + offset + ciphertext_size, VoiceCrypto::TAG_SIZE);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, VoiceCrypto::TAG_SIZE, tag.data());

    // Final
    if (EVP_DecryptFinal_ex(ctx, nullptr, &plaintext_len) != 1) {
      result.error = "EVP_DecryptFinal_ex failed (tag mismatch or corrupt packet)";
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    // Plaintext allocieren
    result.plaintext.resize(plaintext_len);
    if (EVP_DecryptUpdate(ctx, result.plaintext.data(), &len, input.data() + offset, ciphertext_size) != 1) {
      result.error = "EVP_DecryptUpdate failed";
      EVP_CIPHER_CTX_free(ctx);
      return result;
    }

    EVP_CIPHER_CTX_free(ctx);

    result.success = true;
    result.sequence = sequence;
    result.session_id = session_id;
    result.target = target;

    return result;
  }
};

// ── VoiceCrypto ───────────────────────────────────────────────────────

bool VoiceCrypto::initialize(const std::array<uint8_t, KEY_SIZE>& key) {
  impl_ = std::make_unique<Impl>();
  return impl_->initialize(key);
}

VoiceCrypto::EncryptResult VoiceCrypto::encrypt(std::span<const uint8_t> input, uint64_t sequence,
                                                 uint8_t session_id, uint8_t target) {
  if (!impl_) {
    return {};
  }
  return impl_->encrypt(input, sequence, session_id, target);
}

VoiceCrypto::EncryptResult VoiceCrypto::encrypt_with_nonce(std::span<const uint8_t> input,
                                                            const std::array<uint8_t, NONCE_SIZE>& nonce,
                                                            uint8_t session_id, uint8_t target) {
  if (!impl_) {
    return {};
  }
  return impl_->encrypt_with_nonce(input, nonce, session_id, target);
}

VoiceCrypto::DecryptResult VoiceCrypto::decrypt(std::span<const uint8_t> input) {
  if (!impl_) {
    return {};
  }
  return impl_->decrypt(input);
}

void VoiceCrypto::rotate_key() {
  if (!impl_) {
    return;
  }

  std::array<uint8_t, KEY_SIZE> new_key{};
  if (RAND_bytes(new_key.data(), KEY_SIZE) != 1) {
    hh::get_logger().error("VoiceCrypto: Failed to generate random key");
    return;
  }

  impl_->initialize(new_key);
  key_rotated_ = true;
  stats_.key_rotations++;

  hh::get_logger().info("VoiceCrypto: Key rotated");

  if (key_rotation_callback_) {
    key_rotation_callback_(new_key);
  }
}

void VoiceCrypto::set_config(const Config& config) {
  config_ = config;
}

VoiceCrypto::Stats VoiceCrypto::get_stats() const {
  return stats_;
}

void VoiceCrypto::reset_stats() {
  stats_ = {};
}

void VoiceCrypto::set_key_rotation_callback(KeyRotationCallback callback) {
  key_rotation_callback_ = callback;
}

} // namespace hh::server

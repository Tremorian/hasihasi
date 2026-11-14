// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace hh::server {

// Zertifikats-Authentifizierung für Client-Verbindungen
class CertAuth {
public:
  // Zertifikats-Fingerprint (SHA-256)
  using CertFingerprint = std::string;

  // Nutzer-Information nach Authentifizierung
  struct AuthResult {
    bool success;
    std::string error;
    uint32_t user_id;
    std::string username;
    CertFingerprint cert_fingerprint;
    bool is_registered;
    bool is_new_cert; // TOFU: Neues Zertifikat
  };

  // Zertifikats-Validierungsergebnis
  struct CertValidation {
    bool valid;
    std::string error;
    CertFingerprint fingerprint;
    std::string subject;
    std::string issuer;
    bool is_self_signed;
  };

  CertAuth() = default;
  ~CertAuth() = default;

  // Initialisierung
  bool initialize(const std::string& cert_store_path = "");
  void shutdown();

  // Zertifikats-Validierung
  CertValidation validate_certificate(const asio::ssl::X509& cert);
  CertValidation validate_certificate_pem(const std::string& pem);

  // Fingerprint berechnen
  static CertFingerprint calculate_fingerprint(const asio::ssl::X509& cert);
  static CertFingerprint calculate_fingerprint_pem(const std::string& pem);

  // Authentifizierung
  using AuthCallback = std::function<void(const AuthResult&)>;
  void authenticate(const asio::ssl::X509& cert, AuthCallback callback);
  void authenticate_pem(const std::string& pem, AuthCallback callback);

  // TOFU (Trust on First Use)
  bool enable_tofu(bool enabled);
  bool is_tofu_enabled() const { return tofu_enabled_; }

  // Let's Encrypt
  bool enable_lets_encrypt(bool enabled, const std::string& cert_path = "", const std::string& key_path = "");
  bool is_lets_encrypt_enabled() const { return lets_encrypt_enabled_; }

  // Zertifikats-Speicherung
  bool store_certificate(const CertFingerprint& fingerprint, const std::string& username, uint32_t user_id);
  AuthResult find_user_by_certificate(const CertFingerprint& fingerprint);

  // Server-Zertifikat (für TLS)
  bool set_server_certificate(const std::string& cert_path, const std::string& key_path);
  const std::string& server_cert_path() const { return server_cert_path_; }
  const std::string& server_key_path() const { return server_key_path_; }

  // Statistik
  struct Stats {
    size_t certificates_validated;
    size_t authentication_success;
    size_t authentication_failures;
    size_t tofu_accepts;
    size_t tofu_rejects;
  };

  Stats get_stats() const;
  void reset_stats();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  bool tofu_enabled_ = true; // TOFU standardmäßig aktiv
  bool lets_encrypt_enabled_ = false;
  std::string cert_store_path_;
  std::string server_cert_path_;
  std::string server_key_path_;

  Stats stats_;
};

} // namespace hh::server

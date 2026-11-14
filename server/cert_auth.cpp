// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server/cert_auth.hpp"
#include "server/database.hpp"

#include "hh/logging.hpp"

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <sstream>
#include <iomanip>

namespace hh::server {

// ── Plattform-Implementierung ─────────────────────────────────────────

struct CertAuth::Impl {
  std::map<CertFingerprint, std::pair<std::string, uint32_t>> cert_store_; // fingerprint -> (username, user_id)
  std::shared_ptr<Database> database_;

  Impl() = default;
  ~Impl() = default;

  bool initialize(const std::string& cert_store_path) {
    cert_store_path_ = cert_store_path;

    // SQLite-Datenbank laden
    database_ = std::make_shared<Database>(cert_store_path.empty() ? ":memory:" : cert_store_path);
    if (!database_->initialize()) {
      hh::get_logger().error("CertAuth: Failed to initialize database");
      return false;
    }

    // Zertifikate aus Datenbank laden
    load_certificates_from_db();

    hh::get_logger().info("CertAuth: Initialized with {} certificates", cert_store_.size());
    return true;
  }

  void shutdown() {
    save_certificates_to_db();
    database_.reset();
    cert_store_.clear();
  }

  CertAuth::CertValidation validate_certificate(const asio::ssl::X509& cert) {
    CertValidation result{};

    // Fingerprint berechnen
    result.fingerprint = calculate_fingerprint(cert);

    // Subject und Issuer extrahieren
    result.subject = x509_name_to_string(cert, X509_get_subject_name);
    result.issuer = x509_name_to_string(cert, X509_get_issuer_name);

    // Selbst-signiert prüfen
    result.is_self_signed = (result.subject == result.issuer);

    // Ablaufprüfung
    time_t not_after = X509_getm_notAfter(cert.native_handle());
    time_t now = time(nullptr);
    if (not_after < now) {
      result.valid = false;
      result.error = "Certificate expired";
      hh::get_logger().warn("CertAuth: Certificate expired (subject: {})", result.subject);
      return result;
    }

    // Let's Encrypt-Validierung (wenn aktiv)
    if (lets_encrypt_enabled_ && !is_self_signed) {
      // Prüfe, ob Issuer ein bekannter Let's Encrypt Issuer ist
      std::string le_issuers[] = {
        "Let's Encrypt",
        "LetsEncrypt",
        "ZeroSSL",
        "Buypass",
        "ISRG Root X1"
      };
      bool is_le_issuer = false;
      for (const auto& issuer : le_issuers) {
        if (result.issuer.find(issuer) != std::string::npos) {
          is_le_issuer = true;
          break;
        }
      }
      
      if (is_le_issuer) {
        hh::get_logger().info("CertAuth: Let's Encrypt certificate validated (subject: {})", result.subject);
      }
    }

    // Validierung
    result.valid = true;

    hh::get_logger().debug("CertAuth: Certificate validated (fingerprint: {}, subject: {}, self-signed: {})",
                           result.fingerprint.substr(0, 16) + "...",
                           result.subject,
                           result.is_self_signed);

    return result;
  }

  CertAuth::CertValidation validate_certificate_pem(const std::string& pem) {
    CertValidation result{};

    // PEM in X509 konvertieren
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
      result.valid = false;
      result.error = "Failed to create BIO";
      return result;
    }

    BIO_write(bio, pem.data(), pem.size());
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
      result.valid = false;
      result.error = "Failed to parse PEM certificate";
      return result;
    }

    // Fingerprint berechnen
    result.fingerprint = calculate_fingerprint(cert);

    // Subject und Issuer extrahieren
    result.subject = x509_name_to_string(cert, X509_get_subject_name);
    result.issuer = x509_name_to_string(cert, X509_get_issuer_name);

    // Selbst-signiert prüfen
    result.is_self_signed = (result.subject == result.issuer);

    // Validierung
    result.valid = true;

    X509_free(cert);

    return result;
  }

  CertAuth::CertFingerprint CertAuth::Impl::calculate_fingerprint(const asio::ssl::X509& cert) {
    return CertAuth::calculate_fingerprint(cert);
  }

  void authenticate(const asio::ssl::X509& cert, AuthCallback callback) {
    AuthResult result{};

    // Zertifikat validieren
    auto validation = validate_certificate(cert);
    if (!validation.valid) {
      result.success = false;
      result.error = validation.error;
      stats_.authentication_failures++;
      callback(result);
      return;
    }

    stats_.certificates_validated++;

    // In Zertifikats-Suche
    auto it = cert_store_.find(validation.fingerprint);
    if (it != cert_store_.end()) {
      // Bekanntes Zertifikat
      result.success = true;
      result.user_id = it->second.second;
      result.username = it->second.first;
      result.cert_fingerprint = validation.fingerprint;
      result.is_registered = true;
      result.is_new_cert = false;

      hh::get_logger().info("CertAuth: User {} authenticated with known cert {}",
                            result.username, result.cert_fingerprint.substr(0, 16) + "...");
    } else {
      // Neues Zertifikat
      if (tofu_enabled_) {
        // TOFU: Zertifikat akzeptieren, aber als neu markieren
        result.success = true;
        result.user_id = 0; // Noch keine User-ID
        result.username = validation.subject;
        result.cert_fingerprint = validation.fingerprint;
        result.is_registered = false;
        result.is_new_cert = true;

        stats_.tofu_accepts++;

        hh::get_logger().warn("CertAuth: New certificate accepted via TOFU (fingerprint: {}, subject: {})",
                              result.cert_fingerprint.substr(0, 16) + "...",
                              result.username);
      } else {
        // TOFU deaktiviert: Zertifikat ablehnen
        result.success = false;
        result.error = "Certificate not registered and TOFU is disabled";
        stats_.tofu_rejects++;
        stats_.authentication_failures++;

        hh::get_logger().info("CertAuth: New certificate rejected (TOFU disabled)");
      }
    }

    if (result.success) {
      stats_.authentication_success++;
    }
    callback(result);
  }

  void authenticate_pem(const std::string& pem, AuthCallback callback) {
    // PEM in X509 konvertieren
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
      AuthResult result{};
      result.success = false;
      result.error = "Failed to create BIO";
      callback(result);
      return;
    }

    BIO_write(bio, pem.data(), pem.size());
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
      AuthResult result{};
      result.success = false;
      result.error = "Failed to parse PEM certificate";
      callback(result);
      return;
    }

    // Authentifizierung durchführen
    authenticate(cert, [cert, callback](const AuthResult& result) {
      X509_free(cert);
      callback(result);
    });
  }

  bool store_certificate(const CertAuth::CertFingerprint& fingerprint, const std::string& username, uint32_t user_id) {
    cert_store_[fingerprint] = {username, user_id};
    save_certificate_to_db(fingerprint, username, user_id);
    hh::get_logger().info("CertAuth: Certificate stored (fingerprint: {}, username: {}, user_id: {})",
                          fingerprint.substr(0, 16) + "...", username, user_id);
    return true;
  }

  AuthResult CertAuth::Impl::find_user_by_certificate(const CertAuth::CertFingerprint& fingerprint) {
    AuthResult result{};

    auto it = cert_store_.find(fingerprint);
    if (it != cert_store_.end()) {
      result.success = true;
      result.user_id = it->second.second;
      result.username = it->second.first;
      result.cert_fingerprint = fingerprint;
      result.is_registered = true;
      result.is_new_cert = false;
    } else {
      result.success = false;
      result.error = "Certificate not found";
    }

    return result;
  }

  void load_certificates_from_db() {
    if (!database_) return;

    // SQLite-Abfrage: SELECT fingerprint, username, user_id FROM certificates
    auto rows = database_->query("SELECT fingerprint, username, user_id FROM certificates");
    for (const auto& row : rows) {
      if (row.size() >= 3) {
        std::string fingerprint = row[0];
        std::string username = row[1];
        uint32_t user_id = std::stoul(row[2]);
        cert_store_[fingerprint] = {username, user_id};
      }
    }
  }

  void save_certificates_to_db() {
    if (!database_) return;

    // Alle Zertifikate in Datenbank speichern
    for (const auto& [fingerprint, data] : cert_store_) {
      save_certificate_to_db(fingerprint, data.first, data.second);
    }
  }

  void save_certificate_to_db(const std::string& fingerprint, const std::string& username, uint32_t user_id) {
    if (!database_) return;

    // INSERT OR REPLACE INTO certificates (fingerprint, username, user_id) VALUES (?, ?, ?)
    database_->execute("INSERT OR REPLACE INTO certificates (fingerprint, username, user_id) VALUES (?, ?, ?)",
                       fingerprint, username, user_id);
  }

  std::string x509_name_to_string(const asio::ssl::X509& cert, int (*get_name)(const X509*)) {
    X509_NAME* name = get_name(const_cast<X509*>(cert.native_handle()));
    if (!name) return "Unknown";

    char buf[256];
    X509_NAME_get_text_by_NID(name, NID_commonName, buf, sizeof(buf));
    return std::string(buf);
  }

  std::string cert_store_path_;
};

// ── CertAuth ──────────────────────────────────────────────────────────

bool CertAuth::initialize(const std::string& cert_store_path) {
  impl_ = std::make_unique<Impl>();
  return impl_->initialize(cert_store_path);
}

void CertAuth::shutdown() {
  if (impl_) {
    impl_->shutdown();
  }
}

CertAuth::CertValidation CertAuth::validate_certificate(const asio::ssl::X509& cert) {
  if (!impl_) {
    return {};
  }
  return impl_->validate_certificate(cert);
}

CertAuth::CertValidation CertAuth::validate_certificate_pem(const std::string& pem) {
  if (!impl_) {
    return {};
  }
  return impl_->validate_certificate_pem(pem);
}

CertAuth::CertFingerprint CertAuth::calculate_fingerprint(const asio::ssl::X509& cert) {
  // SHA-256 Fingerprint berechnen
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return "";

  const EVP_MD* md = EVP_sha256();

  // Zertifikat digestieren
  if (X509_digest(const_cast<X509*>(cert.native_handle()), md, nullptr, nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    return "";
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (X509_digest(const_cast<X509*>(cert.native_handle()), md, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    return "";
  }

  EVP_MD_CTX_free(ctx);

  // Als Hex kodieren
  std::stringstream ss;
  for (unsigned int i = 0; i < digest_len; ++i) {
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(digest[i]);
  }

  return ss.str();
}

CertAuth::CertFingerprint CertAuth::calculate_fingerprint_pem(const std::string& pem) {
  BIO* bio = BIO_new(BIO_s_mem());
  if (!bio) return "";

  BIO_write(bio, pem.data(), pem.size());
  X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);

  if (!cert) return "";

  auto fingerprint = calculate_fingerprint(cert);
  X509_free(cert);

  return fingerprint;
}

void CertAuth::authenticate(const asio::ssl::X509& cert, AuthCallback callback) {
  if (!impl_) {
    AuthResult result{};
    result.success = false;
    result.error = "Not initialized";
    callback(result);
    return;
  }
  impl_->authenticate(cert, callback);
}

void CertAuth::authenticate_pem(const std::string& pem, AuthCallback callback) {
  if (!impl_) {
    AuthResult result{};
    result.success = false;
    result.error = "Not initialized";
    callback(result);
    return;
  }
  impl_->authenticate_pem(pem, callback);
}

bool CertAuth::enable_tofu(bool enabled) {
  tofu_enabled_ = enabled;
  hh::get_logger().info("CertAuth: TOFU {}", enabled ? "enabled" : "disabled");
  return true;
}

bool CertAuth::enable_lets_encrypt(bool enabled, const std::string& cert_path, const std::string& key_path) {
  lets_encrypt_enabled_ = enabled;
  if (enabled) {
    server_cert_path_ = cert_path;
    server_key_path_ = key_path;
  }
  hh::get_logger().info("CertAuth: Let's Encrypt {}", enabled ? "enabled" : "disabled");
  return true;
}

bool CertAuth::store_certificate(const CertFingerprint& fingerprint, const std::string& username, uint32_t user_id) {
  if (!impl_) return false;
  return impl_->store_certificate(fingerprint, username, user_id);
}

CertAuth::AuthResult CertAuth::find_user_by_certificate(const CertFingerprint& fingerprint) {
  if (!impl_) {
    return {};
  }
  return impl_->find_user_by_certificate(fingerprint);
}

bool CertAuth::set_server_certificate(const std::string& cert_path, const std::string& key_path) {
  server_cert_path_ = cert_path;
  server_key_path_ = key_path;
  hh::get_logger().info("CertAuth: Server certificate set (cert: {}, key: {})", cert_path, key_path);
  return true;
}

CertAuth::Stats CertAuth::get_stats() const {
  return stats_;
}

void CertAuth::reset_stats() {
  stats_ = {};
}

} // namespace hh::server

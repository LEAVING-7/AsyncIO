#pragma once
#include "Async/utils/predefined.hpp"
#include <filesystem>
#include <mutex>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
namespace async {
struct OpenSSLError {
  unsigned long code;
  auto message() const -> std::string
  {
    auto buf = std::array<char, 256> {};
    ERR_error_string_n(code, buf.data(), buf.size());
    return std::string(buf.data());
  }
  inline static auto GetLastErr() -> OpenSSLError { return {ERR_get_error()}; }
  inline static auto GetLastErrMsg() -> std::string { return OpenSSLError {GetLastErr()}.message(); }
};
class TlsContext {
public:
  enum class FileType {
    Asn1 = SSL_FILETYPE_ASN1,
    Pem = SSL_FILETYPE_PEM,
  };

  TlsContext(SSL_CTX* context) : mContext(context) {}
  static auto Create() -> Expected<TlsContext, OpenSSLError>
  {
    {
      auto lk = std::scoped_lock(sSslMutex);
      if (sSslInitCount == 0) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
      }
      ++sSslInitCount;
    }
    auto ctx = SSL_CTX_new(TLS_method());
    if (ctx == nullptr) {
      return make_unexpected(OpenSSLError::GetLastErr());
    }
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_default_verify_paths(ctx);
    return ctx;
  }
  auto use(std::filesystem::path cert, FileType certTy, std::filesystem::path key, FileType keyTy)
      -> Expected<void, OpenSSLError>
  {
    if (auto r = SSL_CTX_use_certificate_file(mContext, cert.c_str(), static_cast<int>(certTy)); !r) {
      return make_unexpected(OpenSSLError::GetLastErr());
    };
    if (auto r = SSL_CTX_use_PrivateKey_file(mContext, key.c_str(), static_cast<int>(keyTy)); !r) {
      return make_unexpected(OpenSSLError::GetLastErr());
    };
    if (auto r = SSL_CTX_check_private_key(mContext); !r) {
      return make_unexpected(OpenSSLError::GetLastErr());
    };
    return {};
  }
  ~TlsContext()
  {
    if (mContext != nullptr) {
      SSL_CTX_free(mContext);
      mContext = nullptr;
    }
  }
  auto raw() -> SSL_CTX* { return mContext; }

private:
  inline static std::uint64_t sSslInitCount {0};
  inline static std::mutex sSslMutex;
  SSL_CTX* mContext;
};
} // namespace async
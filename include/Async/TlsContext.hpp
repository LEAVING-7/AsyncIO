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
  enum FileType {
    Asn1 = SSL_FILETYPE_ASN1,
    Pem = SSL_FILETYPE_PEM,
  };

  static auto Create() -> Expected<TlsContext, OpenSSLError>
  {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    auto ctx = SSL_CTX_new(TLS_method());
    if (ctx == nullptr) {
      return make_unexpected(OpenSSLError::GetLastErr());
    }
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_default_verify_paths(ctx);
    return ctx;
  }

  TlsContext(SSL_CTX* context) : mContext(context) {}
  TlsContext(TlsContext const&) = delete;
  TlsContext(TlsContext&& other) noexcept = default;
  TlsContext& operator=(TlsContext const&) = delete;
  TlsContext& operator=(TlsContext&& other) noexcept = default;
  ~TlsContext() = default;

  auto use(std::filesystem::path cert, FileType certTy, std::filesystem::path key, FileType keyTy)
      -> Expected<void, OpenSSLError>
  {
    if (SSL_CTX_use_certificate_file(raw(), cert.c_str(), certTy) != 1) {
      return make_unexpected(OpenSSLError::GetLastErr());
    }
    if (SSL_CTX_use_PrivateKey_file(raw(), key.c_str(), keyTy) != 1) {
      return make_unexpected(OpenSSLError::GetLastErr());
    };
    if (SSL_CTX_check_private_key(raw()) != 1) {
      return make_unexpected(OpenSSLError::GetLastErr());
    };
    return {};
  }
  auto raw() -> SSL_CTX* { return mContext.get(); }

private:
  inline static std::uint64_t sSslInitCount {0};
  inline static std::mutex sSslMutex;
  struct CtxDeleter {
    void operator()(SSL_CTX* ctx) const { SSL_CTX_free(ctx); }
  };
  std::unique_ptr<SSL_CTX, CtxDeleter> mContext;
};
} // namespace async
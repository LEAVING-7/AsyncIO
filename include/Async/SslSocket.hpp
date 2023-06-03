#pragma once
#include "TlsContext.hpp"
#include "sys/Socket.hpp"

namespace async {
struct SslError {
  inline static auto GetError(SSL* ssl, int r) -> SslError { return {SSL_get_error(ssl, r)}; }
  int code;
  static const SslError Ok;
  static const SslError SysCallError;
  auto ok() -> bool { return SSL_ERROR_NONE == code; }
  auto waitReadable() -> bool { return SSL_ERROR_WANT_READ == code; }
  auto waitWritable() -> bool { return SSL_ERROR_WANT_WRITE == code; }
  auto sysCallError() -> bool { return SSL_ERROR_SYSCALL == code; }
  auto sslError() -> bool { return SSL_ERROR_SSL == code; }
  auto wait() -> bool { return waitReadable() || waitWritable(); }
  auto message() -> std::string_view
  {
    switch (code) {
    case SSL_ERROR_NONE:
      return "SSL_ERROR_NONE";
    case SSL_ERROR_ZERO_RETURN:
      return "SSL_ERROR_ZERO_RETURN";
    case SSL_ERROR_WANT_READ:
      return "SSL_ERROR_WANT_READ";
    case SSL_ERROR_WANT_WRITE:
      return "SSL_ERROR_WANT_WRITE";
    case SSL_ERROR_WANT_CONNECT:
      return "SSL_ERROR_WANT_CONNECT";
    case SSL_ERROR_WANT_ACCEPT:
      return "SSL_ERROR_WANT_ACCEPT";
    case SSL_ERROR_WANT_X509_LOOKUP:
      return "SSL_ERROR_WANT_X509_LOOKUP";
    case SSL_ERROR_SYSCALL:
      return strerror(errno);
    case SSL_ERROR_SSL:
      return "SSL_ERROR_SSL";
    default:
      return "unknown error";
    }
  }
};
inline const SslError SslError::Ok = {SSL_ERROR_NONE};
inline const SslError SslError::SysCallError = {SSL_ERROR_SYSCALL};

class SslSocket {
public:
  friend class SslStream;
  struct SslDeleter {
    void operator()(SSL* ssl) const noexcept { SSL_free(ssl); }
  };
  using SslPtr = std::unique_ptr<SSL, SslDeleter>;

  inline static auto Create(TlsContext& ctx, Socket&& socket) -> Expected<SslSocket, OpenSSLError>
  {
    auto ssl = SSL_new(ctx.raw());
    if (ssl == nullptr) {
      return make_unexpected(OpenSSLError::GetLastErr());
    }
    if (auto r = SSL_set_fd(ssl, socket.getSocket().raw()); r == 0) {
      return make_unexpected(OpenSSLError::GetLastErr());
    };
    return SslSocket(ssl, std::move(socket));
  }
  SslSocket() : mSsl(nullptr) {}
  SslSocket(SSL* ssl, Socket&& socket) : mSocket(std::move(socket)), mSsl(ssl) {}
  SslSocket(SslSocket const&) = delete;
  SslSocket(SslSocket&& other) = default;
  SslSocket& operator=(SslSocket const&) = delete;
  SslSocket& operator=(SslSocket&& other) noexcept = default;
  ~SslSocket() = default;

  auto shutdown() -> std::optional<SslError>
  {
    if (auto r = SSL_shutdown(mSsl.get()); r == 0) { // shutdown not finished
      return std::nullopt;
    } else if (r == 1) {
      return SslError::Ok;
    } else {
      return SslError::GetError(mSsl.get(), r);
    }
  }
  auto setShutdownR() -> void { SSL_set_shutdown(mSsl.get(), SSL_RECEIVED_SHUTDOWN); }
  auto setShutdownW() -> void { SSL_set_shutdown(mSsl.get(), SSL_SENT_SHUTDOWN); }
  auto setShutdownRW() -> void { SSL_set_shutdown(mSsl.get(), SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN); }
  auto defaultShutdown() -> bool
  {
    SSL_set_shutdown(mSsl.get(), SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
    if (auto r = shutdown(); !r) {
      r = shutdown();
      return true;
    } else if (r->ok()) {
      return true;
    } else {
      return false;
    }
  }
  auto send(std::span<std::byte const> data)
  {
    struct WritableAwaiter {
      SslSocket& socket;
      std::span<std::byte const> data;
      Expected<size_t, SslError> result;
      bool suspendedBefore = false;
      bool isReadable = false;
      auto await_ready() -> bool
      {
        auto e = SSL_write(socket.ssl(), data.data(), data.size());
        if (e > 0) {
          result = e;
          return true;
        } else if (e <= 0) {
          auto error = SslError::GetError(socket.ssl(), e);
          if (error.waitReadable()) {
            suspendedBefore = true;
            isReadable = true;
            return false;
          } else if (error.waitWritable()) {
            suspendedBefore = true;
            isReadable = false;
            return false;
          } else {
            result = make_unexpected(error);
            return true;
          }
        }
        assert(0);
      }
      auto await_suspend(std::coroutine_handle<> h) -> void
      {
        if (isReadable) {
          auto r = socket.mSocket.regR(h);
          assert(r);
        } else {
          auto r = socket.mSocket.regW(h);
          assert(r);
        }
      }
      auto await_resume() -> Expected<size_t, SslError>
      {
        if (suspendedBefore) {
          auto e = SSL_write(socket.ssl(), data.data(), data.size());
          if (e > 0) {
            return e;
          } else if (e <= 0) {
            auto error = SslError::GetError(socket.ssl(), e);
            return make_unexpected(error);
          }
        } else {
          return std::move(result);
        }
        assert(0);
      }
    };
    return WritableAwaiter {*this, data};
  }

  auto recv(std::span<std::byte> data)
  {
    struct ReadableAwaiter {
      SslSocket& socket;
      std::span<std::byte> data;
      Expected<size_t, SslError> result;
      bool suspendedBefore = false;
      bool isReadable = false;
      auto await_ready() -> bool
      {
        auto e = SSL_read(socket.ssl(), data.data(), data.size());
        if (e > 0) {
          result = e;
          return true;
        } else if (e <= 0) {
          auto error = SslError::GetError(socket.ssl(), e);
          if (error.waitReadable()) {
            suspendedBefore = true;
            isReadable = true;
            return false;
          } else if (error.waitWritable()) {
            suspendedBefore = true;
            isReadable = false;
            return false;
          } else {
            result = make_unexpected(error);
            return true;
          }
        }
        assert(0);
      }
      auto await_suspend(std::coroutine_handle<> h) -> void
      {
        if (isReadable) {
          auto r = socket.mSocket.regR(h);
          assert(r);
        } else {
          auto r = socket.mSocket.regW(h);
          assert(r);
        }
      }
      auto await_resume() -> Expected<size_t, SslError>
      {
        if (suspendedBefore) {
          auto e = SSL_read(socket.ssl(), data.data(), data.size());
          if (e > 0) {
            return e;
          } else if (e <= 0) {
            auto error = SslError::GetError(socket.ssl(), e);
            return make_unexpected(error);
          }
        } else {
          return std::move(result);
        }
        assert(0);
      }
    };
    return ReadableAwaiter {*this, data};
  }

  auto sendfile(impl::fd_t file, off_t offset, size_t size)
  {
    struct SendfileAwaiter {
      SslSocket& socket;
      impl::fd_t file;
      off_t offset;
      size_t size;
      Expected<size_t, SslError> result;
      bool suspendedBefore = false;
      bool isReadable = false;
      auto await_ready() -> bool
      {
        auto e = SSL_sendfile(socket.ssl(), file, offset, size, 0); // flag is ignored on linux platform
        if (e >= 0) {
          result = e;
          return true;
        } else if (e < 0) {
          auto error = SslError::GetError(socket.ssl(), e);
          if (error.waitReadable()) {
            suspendedBefore = true;
            isReadable = true;
            return false;
          } else if (error.waitWritable()) {
            suspendedBefore = true;
            isReadable = false;
            return false;
          } else {
            result = make_unexpected(error);
            return true;
          }
        }
        assert(0);
      }
      auto await_suspend(std::coroutine_handle<> h) -> void
      {
        if (isReadable) {
          auto r = socket.mSocket.regR(h);
          assert(r);
        } else {
          auto r = socket.mSocket.regW(h);
          assert(r);
        }
      }
      auto await_resume() -> Expected<size_t, SslError>
      {
        if (suspendedBefore) {
          auto e = SSL_sendfile(socket.ssl(), file, offset, size, 0);
          if (e >= 0) {
            return e;
          } else if (e < 0) {
            auto error = SslError::GetError(socket.ssl(), e);
            return make_unexpected(error);
          }
        } else {
          return std::move(result);
        }
        assert(0);
      }
    };
    return SendfileAwaiter {*this, file, offset, size};
  }
  auto sendAll(std::span<std::byte const> buffer) -> Task<Expected<size_t, SslError>>
  {
    while (true) {
      auto n = co_await send(buffer);
      if (n) {
        co_return n;
      } else if (!n && n.error().wait()) {
        continue;
      } else {
        co_return make_unexpected(n.error());
      }
    }
  }
  auto recvAll(std::span<std::byte> buffer) -> Task<Expected<size_t, SslError>>
  {
    while (true) {
      auto n = co_await recv(buffer);
      if (n) {
        co_return n;
      } else if (!n && n.error().wait()) {
        continue;
      } else {
        co_return make_unexpected(n.error());
      }
    }
  }
  auto sendfileAll(impl::fd_t file, off_t offset, size_t size) -> Task<Expected<size_t, SslError>>
  {
    while (true) {
      auto n = co_await sendfile(file, offset, size);
      if (n) {
        if (n.value() < size) {
          size -= n.value();
          offset += n.value();
          continue;
        }
        co_return n;
      } else if (!n && n.error().wait()) {
        continue;
      } else {
        co_return make_unexpected(n.error());
      }
    }
  }
  auto ssl() -> SSL* { return mSsl.get(); }
  auto raw() -> impl::fd_t { return mSocket.getSocket().raw(); }

protected:
  auto accept(TlsContext& ctx, SocketAddr* addr) -> Task<Expected<SslSocket, SslError>>
  {
    // TODO error handling
    auto socket = co_await mSocket.accept(addr);
    assert(socket);
    auto sslSocket = SslSocket::Create(ctx, std::move(socket.value()));
    assert(sslSocket);
    struct AcceptAwaiter {
      SslSocket& socket;
      SslError result;
      bool suspendedBefore = false;
      bool isReadable = false;
      auto await_ready() -> bool
      {
        auto r = SSL_accept(socket.ssl());
        if (r == 1) {
          result = SslError::Ok;
          return true;
        } else {
          auto error = SslError::GetError(socket.ssl(), r);
          if (error.waitReadable()) {
            suspendedBefore = true;
            isReadable = true;
            return false;
          } else if (error.waitWritable()) {
            suspendedBefore = true;
            isReadable = false;
            return false;
          } else { // error occurred
            result = error;
            return true;
          }
        }
      }
      auto await_suspend(std::coroutine_handle<> h) -> void
      {
        if (isReadable) {
          auto r = socket.mSocket.regR(h);
          assert(r);
        } else {
          auto r = socket.mSocket.regW(h);
          assert(r);
        }
      }
      auto await_resume() -> SslError
      {
        if (suspendedBefore) {
          auto r = SSL_accept(socket.ssl());
          if (r == 1) {
            return SslError::Ok;
          } else {
            auto error = SslError::GetError(socket.ssl(), r);
            return error;
          }
        } else {
          return result;
        }
      }
    };
    while (true) {
      auto r = co_await AcceptAwaiter {*sslSocket};
      if (r.ok()) {
        break;
      } else if (r.wait()) {
        continue;
      } else {
        co_return make_unexpected(r);
      }
    }
    co_return std::move(sslSocket).value();
  }

private:
  Socket mSocket;
  SslPtr mSsl {nullptr};
};
} // namespace async
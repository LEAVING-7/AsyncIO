#pragma once
#include "SslSocket.hpp"

#include "TcpStream.hpp"
namespace async {
class SslStream : public SslSocket {
public:
  inline static auto Connect(TlsContext& ctx, async::Reactor& reactor, SocketAddr addr)
      -> Task<Expected<SslStream, SslError>>
  {
    auto r = co_await TcpStream::Connect(reactor, addr);
    assert(r);
    auto sslSocket = SslSocket::Create(ctx, r.value().take());
    struct ConnectAwaiter {
      SslSocket& socket;
      SslError result;
      bool suspendedBefore = false;
      bool isReadable = false;
      auto await_ready() -> bool
      {
        auto r = SSL_connect(socket.ssl());
        if (r == 0) {
          result = SslError::GetError(socket.ssl(), r);
          return true;
        } else if (r == 1) { // connect success
          result = SslError::Ok;
          return true;
        } else if (r < 0) {
          result = SslError::GetError(socket.ssl(), r);
          if (result.waitReadable()) {
            suspendedBefore = true;
            isReadable = true;
            return false;
          } else if (result.waitWritable()) {
            suspendedBefore = true;
            isReadable = false;
            return false;
          } else {
            return true;
          }
        } else {
          assert(0 && "unreachable");
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
          auto r = SSL_connect(socket.ssl());
          if (r == 1) {
            return SslError::Ok;
          } else {
            return SslError::GetError(socket.ssl(), r);
          }
        } else {
          return result;
        }
      }
    };
    while (true) {
      auto r = co_await ConnectAwaiter {*sslSocket};
      if (r.ok()) {
        break;
      } else if (r.wait()) {
        if (r.waitReadable()) {
          continue;
        } else if (r.waitWritable()) {
          continue;
        }
        continue;
      } else {
        co_return make_unexpected(r);
      }
    }
    co_return SslStream {std::move(sslSocket).value()};
  }
  SslStream() = default;
  SslStream(SslSocket&& socket) : SslSocket(std::move(socket)) {}
  SslStream(SslStream const&) = delete;
  SslStream(SslStream&& other) noexcept = default;
  SslStream& operator=(SslStream const&) = delete;
  SslStream& operator=(SslStream&& other) noexcept = default;
  ~SslStream() = default;
};
} // namespace async
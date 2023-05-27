#pragma once
#include "SslSocket.hpp"
#include "TcpListener.hpp"
namespace async {
class SslListener : public SslSocket {
public:
  inline static auto Bind(TlsContext& ctx, async::Reactor& reactor, SocketAddr const& addr)
      -> Expected<SslListener, std::string>
  {
    auto listener = TcpListener::Bind(reactor, addr);
    if(!listener) {
      return make_unexpected(strerror(int(listener.error())));
    }
    auto sslSocket = SslSocket::Create(ctx, listener.value().take());
    if(!sslSocket) {
      return make_unexpected(sslSocket.error().message());
    }
    auto socket = std::move(sslSocket).value();
    return SslListener(std::move(socket));
  }
  SslListener() = default;
  SslListener(SslSocket&& socket) : SslSocket(std::move(socket)) {}
  SslListener(SslListener const&) = delete;
  SslListener(SslListener&& other) noexcept = default;
  SslListener& operator=(SslListener const&) = delete;
  SslListener& operator=(SslListener&& other) noexcept = default;
  ~SslListener() = default;

  auto accept(TlsContext& ctx, SocketAddr* addr) { return SslSocket::accept(ctx, addr); }
};
} // namespace async
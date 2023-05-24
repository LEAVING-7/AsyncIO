#pragma once
#include "SslSocket.hpp"
#include "TcpListener.hpp"
namespace async {
class SslListener : public SslSocket {
public:
  inline static auto Bind(TlsContext& ctx, async::Reactor& reactor, SocketAddr const& addr) -> SslListener
  {
    // TODO: error handling
    auto listener = TcpListener::Bind(reactor, addr);
    assert(listener);
    auto sslSocket = SslSocket::Create(ctx, listener.value().take());
    assert(sslSocket);
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
};
} // namespace async
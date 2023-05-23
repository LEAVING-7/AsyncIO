#pragma once
#include "SslSocket.hpp"
#include "TcpListener.hpp"
namespace async {
class SslListener {
  inline static auto Bind(TslContext& ctx, async::Reactor& reactor, SocketAddr const& addr) -> SslListener
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
  SslListener(SslSocket&& socket) : mSocket(std::move(socket)) {}
  SslListener(SslListener const&) = delete;
  SslListener(SslListener&& other) noexcept : mSocket(std::move(other.mSocket)) {}
  SslListener& operator=(SslListener const&) = delete;
  SslListener& operator=(SslListener&& other) noexcept = default;
  ~SslListener() = default;

  auto accept(TslContext& ctx, SocketAddr* addr) { return mSocket.accept(ctx, addr); }
  auto shutdown() { return mSocket.shutdown(); }
  auto sendAll(std::span<std::byte const> buffer) -> Task<Expected<size_t, SslError>>
  {
    while (true) {
      auto n = co_await mSocket.send(buffer);
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
      auto n = co_await mSocket.recv(buffer);
      if (n) {
        co_return n;
      } else if (!n && n.error().wait()) {
        continue;
      } else {
        co_return make_unexpected(n.error());
      }
    }
  }

  auto recv(std::span<std::byte> buffer) { return mSocket.recv(buffer); }
  auto send(std::span<std::byte const> buffer) { return mSocket.send(buffer); }

private:
  async::SslSocket mSocket;
};
} // namespace async
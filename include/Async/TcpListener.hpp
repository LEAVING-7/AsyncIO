#pragma once
#include "Async/Reactor.hpp"
#include "Async/Task.hpp"

#include "TcpStream.hpp"
#include "sys/Socket.hpp"

namespace async {
class TcpListener {
public:
  inline static auto Bind(async::Reactor& reactor, SocketAddr const& addr) -> StdResult<TcpListener>
  {
    if (auto socket = impl::Socket::CreateNonBlock(addr); !socket) {
      return make_unexpected(socket.error());
    } else if (auto r = socket->bind(addr); !r) {
      return make_unexpected(r.error());
    } else if (auto r = socket->listen(1024); !r) {
      return make_unexpected(r.error());
    } else {
      if (auto source = reactor.insertIo(socket->raw()); !source) {
        return make_unexpected(source.error());
      } else {
        return {TcpListener {&reactor, *source}};
      }
    }
  }

  TcpListener() : mSocket() {};
  TcpListener(async::Reactor* reactor, std::shared_ptr<async::Source> source) : mSocket(reactor, source) {}
  TcpListener(TcpListener const&) = delete;
  TcpListener(TcpListener&&) = default;
  ~TcpListener() = default;

  auto accept(SocketAddr* addr) { return mSocket.accept(addr); }
  auto write(std::span<std::byte const> data) { return mSocket.send(data); }
  auto read(std::span<std::byte> data) { return mSocket.recv(data); }

  auto getSocket() const -> impl::Socket { return mSocket.getSocket(); }
  auto raw() const -> impl::fd_t { return mSocket.getSocket().raw(); }
  auto take() -> async::Socket { return std::move(mSocket); }

private:
  async::Socket mSocket;
};
} // namespace async
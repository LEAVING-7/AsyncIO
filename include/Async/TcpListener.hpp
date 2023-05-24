#pragma once
#include "Async/Reactor.hpp"
#include "Async/Task.hpp"

#include "TcpStream.hpp"
#include "sys/Socket.hpp"

namespace async {
class TcpListener : public Socket {
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

  TcpListener() = default;
  TcpListener(async::Reactor* reactor, std::shared_ptr<async::Source> source) : Socket(reactor, source) {}
  TcpListener(TcpListener const&) = delete;
  TcpListener(TcpListener&&) = default;
  ~TcpListener() = default;

  auto raw() const -> impl::fd_t { return getSocket().raw(); }
  auto take() -> async::Socket { return Socket(mReactor, std::move(mSource)); }
};
} // namespace async
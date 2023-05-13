#pragma once
#include "Async/Reactor.hpp"
#include "Async/Task.hpp"

#include "TcpStream.hpp"
#include "sys/SocketAddr.hpp"
#include "sys/sys.hpp"

#include <stacktrace>

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

  TcpListener() : mSource(nullptr), mReactor(nullptr) {};
  TcpListener(async::Reactor* reactor, std::shared_ptr<async::Source> source)
      : mSource(std::move(source)), mReactor(reactor)
  {
  }
  TcpListener(TcpListener const&) = delete;
  TcpListener(TcpListener&&) = default;
  ~TcpListener()
  {
    if (mSource) {
      assert(mReactor);
      assert(mReactor->removeIo(*mSource));
      assert(getSocket().close());
    }
  }

  auto accept(SocketAddr* addr) -> Task<StdResult<TcpStream>>
  {
    auto socket = getSocket().acceptNonBlock(addr);
    if (!socket) {
      if (socket.error() == std::errc::operation_would_block ||
          socket.error() == std::errc::resource_unavailable_try_again) {
        co_await mReactor->readable(mSource);
        socket = getSocket().acceptNonBlock(addr);
        if (!socket) {
          co_return make_unexpected(socket.error());
        } else {
          auto source = mReactor->insertIo(socket->raw());
          assert(source);
          co_return {TcpStream {mReactor, *source}};
        }
      } else {
        co_return make_unexpected(socket.error());
      }
    } else {
      auto source = mReactor->insertIo(socket->raw());
      assert(source);
      co_return {TcpStream {mReactor, *source}};
    }
  }

  auto write(std::span<std::byte const> data) -> Task<StdResult<ssize_t>>
  {
    auto n = getSocket().sendNonBlock(data, 0);
    if (!n) {
      if (n.error() == std::errc::operation_would_block || n.error() == std::errc::resource_unavailable_try_again) {
        co_await mReactor->writable(mSource);
        co_return getSocket().sendNonBlock(data, 0);
      } else {
        co_return make_unexpected(n.error());
      }
    } else {
      co_return n;
    }
  }
  auto read(std::span<std::byte> data) -> Task<StdResult<ssize_t>>
  {
    auto n = getSocket().recvNonBlock(data, 0);
    if (!n) {
      if (n.error() == std::errc::operation_would_block || n.error() == std::errc::resource_unavailable_try_again) {
        co_await mReactor->readable(mSource);
        co_return getSocket().recvNonBlock(data, 0);
      } else {
        co_return make_unexpected(n.error());
      }
    } else {
      co_return n;
    }
  }
  auto getSocket() const -> impl::Socket
  {
    assert(mSource);
    return impl::Socket(mSource->fd);
  }

private:
  std::shared_ptr<async::Source> mSource;
  async::Reactor* mReactor;
};
} // namespace async
#pragma once
#include "Async/Reactor.hpp"
#include "Async/Task.hpp"

#include "sys/SocketAddr.hpp"
#include "sys/sys.hpp"
namespace async {
class TcpStream {
public:
  inline static auto Connect(async::Reactor& reactor, SocketAddr const& addr) -> Task<StdResult<TcpStream>>
  {
    if (auto sr = impl::Socket::CreateNonBlock(addr); !sr) {
      co_return make_unexpected(sr.error());
    } else if (auto r = sr->connect(addr); !r && r.error() == std::errc::operation_in_progress) {
      // unix failed with EAGAIN
      // tcp failed with EINPROGRESS
      if (auto source = reactor.insertIo(sr->raw()); !source) {
        co_return make_unexpected(source.error());
      } else {
        co_await reactor.writable(source.value());
        auto r = sr->connect(addr);
        assert(r);
        co_return {TcpStream {&reactor, *source}};
      }
    } else if (r.has_value()) { // success
      if (auto source = reactor.insertIo(sr->raw()); !source) {
        co_return make_unexpected(source.error());
      } else {
        co_return {TcpStream {&reactor, *source}};
      }
    } else { // error
      co_return make_unexpected(r.error());
    }
  }

  TcpStream() : mSource(nullptr), mReactor(nullptr) {};
  TcpStream(async::Reactor* reactor, std::shared_ptr<async::Source> source)
      : mSource(std::move(source)), mReactor(reactor)
  {
  }

  TcpStream(TcpStream const&) = delete;
  TcpStream(TcpStream&&) = default;
  TcpStream& operator=(TcpStream&& stream) = default;
  ~TcpStream()
  {
    if (mSource) {
      assert(mReactor);
      assert(mReactor->removeIo(*mSource));
      assert(getSocket().close());
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
    return impl::Socket {mSource->fd};
  }

private:
  std::shared_ptr<async::Source> mSource;
  async::Reactor* mReactor;
};
} // namespace async

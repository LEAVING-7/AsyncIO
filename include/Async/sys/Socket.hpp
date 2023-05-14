#pragma once
#include "sys.hpp"
#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>

namespace async {
class Socket {
public:
  inline static auto Create(Reactor* reactor, SocketAddr const& addr) -> StdResult<Socket>
  {
    if (auto fd = impl::Socket::CreateNonBlock(addr); !fd) {
      return make_unexpected(fd.error());
    } else {
      if (auto source = reactor->insertIo(fd.value().raw()); !source) {
        return make_unexpected(source.error());
      } else {
        return {Socket {reactor, *source}};
      }
    }
  }
  Socket() : mSource(nullptr), mReactor(nullptr) {}
  Socket(Reactor* reactor, std::shared_ptr<Source> source) : mSource(std::move(source)), mReactor(reactor) {}

  Socket(Socket const&) = delete;
  Socket& operator=(Socket const&) = delete;
  Socket(Socket&&) = default;
  Socket& operator=(Socket&&) = default;
  ~Socket()
  {
    if (mSource) {
      assert(mReactor);
      assert(mReactor->removeIo(*mSource));
      assert(getSocket().close());
    }
  }

  auto send(std::span<std::byte const> data)
  {
    struct WritableAwaiter {
      Socket& socket;
      std::span<std::byte const> data;
      StdResult<ssize_t> result;
      bool suspendedBefore = false;
      auto await_ready() noexcept -> bool
      {
        auto n = socket.getSocket().sendNonBlock(data, 0);
        if (!n &&
            (n.error() == std::errc::operation_would_block || n.error() == std::errc::resource_unavailable_try_again)) {
          suspendedBefore = true;
          return false; // suspend now
        } else if (!n) {
          result = make_unexpected(n.error());
          return true;
        } else {
          result = n;
          return true;
        }
      }
      auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { assert(socket.regWritable(handle)); }
      auto await_resume() -> StdResult<ssize_t>
      {
        if (suspendedBefore) { //
          auto n = socket.getSocket().sendNonBlock(data, 0);
          if (!n) {
            return make_unexpected(n.error());
          } else {
            return n;
          }
        } else {
          return std::move(result);
        }
      }
    };
    return WritableAwaiter {*this, data};
  }
  auto recv(std::span<std::byte> data)
  {
    struct ReadableAwaiter {
      Socket& socket;
      std::span<std::byte> data;
      StdResult<ssize_t> result;
      bool suspendedBefore = false; // assign true when suspended
      auto await_ready() noexcept -> bool
      {
        auto n = socket.getSocket().recvNonBlock(data, 0);
        if (!n &&
            (n.error() == std::errc::operation_would_block || n.error() == std::errc::resource_unavailable_try_again)) {
          suspendedBefore = true;
          return false; // suspend right now
        } else if (!n) {
          result = make_unexpected(n.error());
          return true;
        } else {
          result = n;
          return true;
        }
      }
      auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { assert(socket.regReadable(handle)); }
      auto await_resume() -> StdResult<ssize_t>
      {
        if (suspendedBefore) { //
          auto n = socket.getSocket().recvNonBlock(data, 0);
          if (!n) {
            return make_unexpected(n.error());
          } else {
            return n;
          }
        } else {
          return std::move(result);
        }
      }
    };
    return ReadableAwaiter {*this, data};
  }
  // readable
  auto accept(SocketAddr* addr)
  {
    struct AcceptAwaiter {
      Socket& socket;
      SocketAddr* addr;
      StdResult<Socket> result {};
      bool suspendedBefore = false; // assign true when suspended
      auto await_ready() noexcept -> bool
      {
        auto sock = socket.getSocket().acceptNonBlock(addr);
        if (!sock) {
          if (sock.error() == std::errc::operation_would_block ||
              sock.error() == std::errc::resource_unavailable_try_again) {
            suspendedBefore = true;
            return false; // suspend right now
          } else {
            result = make_unexpected(sock.error()); // error occurred
            return true;
          }
        } else {
          result = socket.regSocket(sock.value());
          return true;
        }
      }
      auto await_suspend(std::coroutine_handle<> handle) noexcept -> void { assert(socket.regReadable(handle)); }
      auto await_resume() -> StdResult<Socket>
      {
        if (suspendedBefore) { //
          auto sock = socket.getSocket().acceptNonBlock(addr);
          assert(sock);
          return socket.regSocket(sock.value());
        } else {
          return std::move(result);
        }
      }
    };
    return AcceptAwaiter {*this, addr};
  }
  auto regReadable(std::coroutine_handle<> handle) -> StdResult<>
  {
    if (mSource->setReadable(handle)) {
      return mReactor->updateIo(*mSource);
    } else {
      assert(0 && "already readable");
      return {};
    }
  }
  auto regWritable(std::coroutine_handle<> handle) -> StdResult<>
  {
    if (mSource->setWritable(handle)) {
      return mReactor->updateIo(*mSource);
    } else {
      assert(0 && "already writable");
      return {};
    }
  }
  auto regSocket(impl::Socket socket) -> StdResult<Socket>
  {
    if (auto source = mReactor->insertIo(socket.raw()); !source) {
      return make_unexpected(source.error());
    } else {
      return Socket {mReactor, *source};
    }
  }
  auto getSocket() const -> impl::Socket
  {
    assert(mSource);
    return impl::Socket(mSource->fd);
  }

private:
  std::shared_ptr<Source> mSource;
  Reactor* mReactor;
};
} // namespace async
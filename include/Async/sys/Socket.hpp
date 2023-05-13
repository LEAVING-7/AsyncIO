#pragma once
#include "sys.hpp"
#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>

namespace async {
class Socket {
public:
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
      bool suspendedBefore = false; // assign true when suspended
      auto await_ready() noexcept -> bool
      {
        auto n = socket.getSocket().sendNonBlock(data, 0);
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
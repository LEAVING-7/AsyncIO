#pragma once
#include "Async/Reactor.hpp"
#include "Async/Task.hpp"

#include "sys/Socket.hpp"

namespace async {
class TcpStream : public Socket {
public:
  inline static auto Connect(async::Reactor& reactor, SocketAddr const& addr)
  {
    struct ConnectAwaiter {
      StdResult<Socket> result;
      async::Reactor& reactor;
      SocketAddr const& addr;
      bool suspendedBefore = false;
      auto await_ready() noexcept -> bool
      {
        if (auto r = Socket::Create(&reactor, addr); !r) {
          result = make_unexpected(r.error());
          return true;
        } else {
          // socket create success
          if (auto cr = r->getSocket().connect(addr); !cr) {
            if (cr.error() == std::errc::operation_in_progress) {
              // unix failed with EAGAIN
              // tcp failed with EINPROGRESS
              suspendedBefore = true; // suspend now
              result = std::move(r).value();
              return false;
            } else {
              result = make_unexpected(cr.error());
              return true;
            }
          } else { // connect success
            result = std::move(r).value();
            return true;
          }
        }
      }
      auto await_suspend(std::coroutine_handle<> handle) -> void
      {
        assert(suspendedBefore);
        assert(result);
        auto r = result->regW(handle);
        assert(r);
      }
      auto await_resume() -> StdResult<TcpStream>
      {
        if (suspendedBefore) {
          assert(result);
          // connect again
          if (auto b = result->getSocket().connect(addr); !b) {
            return make_unexpected(b.error()); // connect error
          } else {
            return {TcpStream(std::move(result).value())};
          }
        } else if (result) { // socket created
          return {TcpStream(std::move(result).value())};
        } else { // error occurred
          return make_unexpected(result.error());
        }
      }
    };
    return ConnectAwaiter {{}, reactor, addr};
  }

  TcpStream() = default;
  TcpStream(Socket&& socket) : Socket(std::move(socket)) {}
  TcpStream(async::Reactor* reactor, std::shared_ptr<async::Source> source) : Socket(reactor, source) {}
  TcpStream(TcpStream const&) = delete;
  TcpStream(TcpStream&&) = default;
  TcpStream& operator=(TcpStream&& stream) = default;
  ~TcpStream() = default;

  auto take() -> async::Socket { return Socket {mReactor, std::move(mSource)}; }
};
} // namespace async

#pragma once
#include "Async/Reactor.hpp"
#include "Async/Task.hpp"

#include "sys/Socket.hpp"

namespace async {
class TcpStream {
public:
  inline static auto Connect(async::Reactor& reactor, SocketAddr const& addr)
  {
    // if (auto sr = impl::Socket::CreateNonBlock(addr); !sr) {
    //   return make_unexpected(sr.error());
    // } else if (auto r = sr->connect(addr); !r && r.error() == std::errc::operation_in_progress) {
    //   // unix failed with EAGAIN
    //   // tcp failed with EINPROGRESS
    //   if (auto source = reactor.insertIo(sr->raw()); !source) {
    //     return make_unexpected(source.error());
    //   } else {
    //     co_await reactor.writable(source.value());
    //     auto r = sr->connect(addr);
    //     assert(r);
    //     return {TcpStream {&reactor, *source}};
    //   }
    // } else if (r.has_value()) { // success
    //   if (auto source = reactor.insertIo(sr->raw()); !source) {
    //     return make_unexpected(source.error());
    //   } else {
    //     return {TcpStream {&reactor, *source}};
    //   }
    // } else { // error
    //   return make_unexpected(r.error());
    // }
    struct ConnectAwaiter {
      StdResult<TcpStream> result;
      async::Reactor& reactor;
      SocketAddr const& addr;

      bool suspendedBefore = false;
      auto await_ready() noexcept -> bool
      {
        if (auto r = impl::Socket::CreateNonBlock(addr); !r) {
          result = make_unexpected(r.error());
          return true;
        } else if (auto b = r->connect(addr); !b) {
          // unix failed with EAGAIN
          // tcp failed with EINPROGRESS
          if (b.error() == std::errc::operation_in_progress) {
            suspendedBefore = true; // suspend now
            return false;
          } else {
            result = make_unexpected(b.error());
            return true;
          }
        } else {
          if (auto source = reactor.insertIo(r->raw()); !source) {
            result = make_unexpected(source.error());
            return true;
          } else {
            result = {TcpStream {&reactor, *source}};
            return true;
          }
        }
      }
    };
  }

  TcpStream() : mSocket() {};
  TcpStream(async::Reactor* reactor, std::shared_ptr<async::Source> source) : mSocket(reactor, source) {}

  TcpStream(TcpStream const&) = delete;
  TcpStream(TcpStream&&) = default;
  TcpStream& operator=(TcpStream&& stream) = default;
  ~TcpStream() = default;

  auto write(std::span<std::byte const> data) { return mSocket.send(data); }
  auto read(std::span<std::byte> data) { return mSocket.recv(data); }

  auto getSocket() const -> impl::Socket { return mSocket.getSocket(); }

private:
  async::Socket mSocket;
};
} // namespace async

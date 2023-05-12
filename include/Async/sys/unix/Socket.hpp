#pragma once
#include "../SocketAddr.hpp"
#include "Async/utils/platform.hpp"
#ifdef UNIX_PLATFORM
  #include "Async/utils/predefined.hpp"
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
namespace async::impl {
using fd_t = int;
constexpr fd_t INVALID_FD = -1;

using sa_family = sa_family_t;
using addrinfo = struct addrinfo;
using sockaddr = struct sockaddr;
using sockaddr_storage = struct sockaddr_storage;
using socketlen_t = socklen_t;

auto SocketAddrToSockAddr(SocketAddr const& addr, impl::sockaddr_storage* storage, impl::socketlen_t* len)
    -> StdResult<void>;

class Socket {
public:
  static auto Create(async::SocketAddr const& addr, int ty) -> StdResult<Socket>;
  static auto CreateNonBlock(async::SocketAddr const& addr) -> StdResult<Socket>;
  Socket() : mFd(INVALID_FD) {}
  Socket(fd_t fd) : mFd(fd) {}
  ~Socket() = default;

  auto connect(SocketAddr const& addr) -> StdResult<void>
  {
    auto storage = sockaddr_storage {};
    auto len = socketlen_t {};
    if (auto r = SocketAddrToSockAddr(addr, &storage, &len); !r) {
      return make_unexpected(r.error());
    }
    if (auto r = SysCall(::connect, mFd, (sockaddr const*)&storage, len); !r) {
      return make_unexpected(r.error());
    };
    return {};
  }

  auto bind(SocketAddr const& addr) -> StdResult<void>
  {
  #ifndef WIN_PLATFORM
    int reuse = 1;
    ::setsockopt(mFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  #endif
    sockaddr_storage storage;
    socklen_t len;
    if (auto r = SocketAddrToSockAddr(addr, &storage, &len); !r) {
      return make_unexpected(r.error());
    }
    if (auto r = SysCall(::bind, mFd, (sockaddr const*)&storage, len); !r) {
      return make_unexpected(r.error());
    }

    return {};
  }

  auto listen(int backlog) -> StdResult<void>
  {
    if (auto r = SysCall(::listen, mFd, backlog); !r) {
      return make_unexpected(r.error());
    }
    return {};
  }

  auto accept(SocketAddr* addr) -> StdResult<Socket>
  {
    if (addr == nullptr) {
      if (auto r = SysCall(::accept, mFd, nullptr, nullptr); !r) {
        return make_unexpected(r.error());
      } else {
        return Socket(r.value());
      }
    } else {
      assert(0 && "unimplemented");
      return {};
    }
  }
  auto acceptNonBlock(SocketAddr* addr) -> StdResult<Socket>
  {
    if (addr == nullptr) {
      if (auto r = SysCall(::accept4, mFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC); !r) {
        return make_unexpected(r.error());
      } else {
        return Socket(r.value());
      }
    } else {
      assert(0 && "unimplemented");
      return {};
    }
  }
  auto send(void const* buf, size_t len, int flags) -> StdResult<int> { return SysCall(::send, mFd, buf, len, flags); }
  auto recv(void* buf, size_t len, int flags) -> StdResult<int> { return SysCall(::recv, mFd, buf, len, flags); }
  auto sendto(void const* buf, size_t len, int flags, sockaddr const* dest_addr, socklen_t addrlen) -> StdResult<int>
  {
    return SysCall(::sendto, mFd, buf, len, flags, dest_addr, addrlen);
  }
  auto recvfrom(void* buf, size_t len, int flags, sockaddr* src_addr, socklen_t* addrlen) -> StdResult<int>
  {
    return SysCall(::recvfrom, mFd, buf, len, flags, src_addr, addrlen);
  }
  auto close() -> StdResult<int> { return SysCall(::close, mFd); }

  auto valid() const -> bool { return mFd != INVALID_FD; }
  auto raw() const -> fd_t { return mFd; }

private:
  fd_t mFd;
};

} // namespace async::impl
#endif
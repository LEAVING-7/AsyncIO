#pragma once
#include "../SocketAddr.hpp"
#ifdef __linux__
  #include "Async/utils/predefined.hpp"
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <span>
  #include <sys/sendfile.h>
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
  auto sendNonBlock(std::span<std::byte const> const buf, int flags) -> StdResult<ssize_t>
  {
    return send(buf.data(), buf.size(), flags | MSG_DONTWAIT);
  }
  auto send(std::span<std::byte const> const buf, int flags) -> StdResult<ssize_t>
  {
    return send(buf.data(), buf.size(), flags);
  }
  auto send(void const* buf, size_t len, int flags) -> StdResult<ssize_t>
  {
    return SysCall(::send, mFd, buf, len, flags);
  }
  auto recvNonBlock(std::span<std::byte> const buf, int flags) -> StdResult<ssize_t>
  {
    return recv(buf.data(), buf.size(), flags | MSG_DONTWAIT);
  }
  auto recv(std::span<std::byte> const buf, int flags) -> StdResult<ssize_t>
  {
    return recv(buf.data(), buf.size(), flags);
  }
  auto recv(void* buf, size_t len, int flags) -> StdResult<ssize_t> { return SysCall(::recv, mFd, buf, len, flags); }
  auto sendto(std::span<std::byte const> const buf, int flags, sockaddr const* dest_addr, socklen_t addrlen)
      -> StdResult<ssize_t>
  {
    return sendto(buf.data(), buf.size(), flags, dest_addr, addrlen);
  }
  auto sendto(void const* buf, size_t len, int flags, sockaddr const* dest_addr, socklen_t addrlen)
      -> StdResult<ssize_t>
  {
    return SysCall(::sendto, mFd, buf, len, flags, dest_addr, addrlen);
  }
  auto recvfrom(std::span<std::byte> const buf, int flags, sockaddr* src_addr, socklen_t* addrlen) -> StdResult<ssize_t>
  {
    return recvfrom(buf.data(), buf.size(), flags, src_addr, addrlen);
  }
  auto recvfrom(void* buf, size_t len, int flags, sockaddr* src_addr, socklen_t* addrlen) -> StdResult<ssize_t>
  {
    return SysCall(::recvfrom, mFd, buf, len, flags, src_addr, addrlen);
  }
  auto shutdownRead() -> StdResult<void> { return shutdown(SHUT_RD); }
  auto shutdownWrite() -> StdResult<void> { return shutdown(SHUT_WR); }
  auto shutdownReadWrite() -> StdResult<void> { return shutdown(SHUT_RDWR); }
  auto sendfile(fd_t inFile, off_t* offset, size_t count) -> StdResult<ssize_t>
  {
    return SysCall(::sendfile, mFd, inFile, offset, count);
  }
  auto close() -> StdResult<int>
  {
    if (auto r = SysCall(::close, mFd); !r) {
      return make_unexpected(r.error());
    } else {
      mFd = INVALID_FD;
      return r;
    };
  }
  auto valid() const -> bool { return mFd != INVALID_FD; }
  auto raw() const -> fd_t { return mFd; }

private:
  auto shutdown(int how) -> StdResult<void>
  {
    if (auto r = SysCall(::shutdown, mFd, how); !r) {
      return make_unexpected(r.error());
    }
    return {};
  }
  fd_t mFd;
};

} // namespace async::impl
#endif
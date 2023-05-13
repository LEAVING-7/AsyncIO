#include <Async/sys/unix/Socket.hpp>

namespace async::impl {
auto Socket::Create(async::SocketAddr const& addr, int ty) -> StdResult<Socket>
{
  if (addr.isIpv4()) {
    return SysCall(::socket, AF_INET, ty | SOCK_STREAM, 0).map([](auto fd) { return Socket(fd); });
  } else if (addr.isIpv6()) {
  }
  assert(0 && "unimplemented");
  return {};
}
auto Socket::CreateNonBlock(async::SocketAddr const& addr) -> StdResult<Socket>
{
  return Socket::Create(addr, SOCK_NONBLOCK | SOCK_CLOEXEC);
}

auto SocketAddrToSockAddr(SocketAddr const& addr, impl::sockaddr_storage* storage, impl::socketlen_t* len)
    -> StdResult<void>
{
  if (addr.isIpv4()) {
    storage->ss_family = AF_INET;
    auto& v4 = addr.getIpv4();
    auto v4Storage = reinterpret_cast<sockaddr_in*>(storage);
    v4Storage->sin_port = htons(v4.port);
    v4Storage->sin_addr.s_addr = *reinterpret_cast<uint32_t const*>(v4.addr.addr);
    *len = sizeof(sockaddr_in);
    return {};
  } else {
    assert(0 && "unimplemented");
    return {};
  }
}
} // namespace async::impl
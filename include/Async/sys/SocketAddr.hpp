#pragma once
#include <cassert>
#include <cstdint>
#include <string>
namespace async {
struct Ipv4Addr {
  operator uint32_t() const { return *reinterpret_cast<uint32_t const*>(addr); }
  uint8_t addr[4];
};

constexpr auto LOCAL_HOST_V4 = Ipv4Addr {127, 0, 0, 1};
constexpr auto BOARDCAST_V4 = Ipv4Addr {255, 255, 255, 255};
constexpr auto ANY_V4 = Ipv4Addr {0, 0, 0, 0};

struct Ipv6Addr {
  uint8_t addr[16];
};

struct SocketAddrV4 {
  Ipv4Addr addr;
  uint16_t port;
};

struct SocketAddrV6 {
  Ipv6Addr addr;
  uint16_t port;
};

class SocketAddr {
public:
  enum class Family : uint8_t {
    V4,
    V6,
  };
  constexpr SocketAddr(SocketAddrV4 addr) : v4(addr), family(Family::V4) {}
  constexpr SocketAddr(SocketAddrV6 addr) : v6(addr), family(Family::V6) {}
  constexpr auto isIpv4() const -> bool { return family == Family::V4; }
  constexpr auto isIpv6() const -> bool { return family == Family::V6; }
  constexpr auto getIpv4() const -> SocketAddrV4 const&
  {
    assert(isIpv4());
    return v4;
  }
  constexpr auto getIpv6() const -> SocketAddrV6 const&
  {
    assert(isIpv6());
    return v6;
  }
  constexpr auto getFamily() const -> Family { return family; }
  auto toString() -> std::string
  {
    if (isIpv4()) {
      auto result = std::string {};
      for (int i = 0; i < 4; i++) {
        result += std::to_string(v4.addr.addr[i]);
        if (i != 3) {
          result += '.';
        }
      }
      result += ':';
      result += std::to_string(v4.port);
      return result;
    } else if (isIpv6()) {
      assert(0 && "unimplmented");
    } else {
      assert(0 && "unreachable");
    }
    return {};
  }

private:
  union {
    SocketAddrV4 v4;
    SocketAddrV6 v6;
  };
  Family family;
};

} // namespace async
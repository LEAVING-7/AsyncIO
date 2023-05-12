#include <Async/sys/SocketAddr.hpp>
#include <Async/sys/unix/Socket.hpp>
#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

TEST(SocketAddrTest, SocketAddrV4)
{
  auto addr = async::SocketAddr(async::SocketAddrV4 {{127, 0, 0, 1}, 8080});
  EXPECT_TRUE(addr.isIpv4());
  auto string = addr.toString();
  EXPECT_EQ(string, "127.0.0.1:8080");
  auto addrStr = "127.0.0.1";
  auto addr2 = uint32_t {0};
  auto port2 = uint16_t {8080};
  auto r = inet_pton(AF_INET, addrStr, &addr2);
  EXPECT_NE(r, -1);
  EXPECT_EQ(uint32_t(addr.getIpv4().addr), inet_addr(addrStr));
  EXPECT_EQ(std::string(inet_ntoa({addr2})), std::string(string.data(), 9));
}
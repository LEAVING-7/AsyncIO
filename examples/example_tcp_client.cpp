#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/TcpStream.hpp>
#include <cstring>
static auto gExecutor = async::MultiThreadExecutor(4);
static auto gReactor = async::Reactor();

int main()
{
  gExecutor.block(
      []() -> async::Task<> {
        for (int i = 0; i < 1000; i++) {
          gExecutor.spawnDetach(
              [](int i) -> async::Task<> {
                auto stream = co_await async::TcpStream::Connect(
                    gReactor, async::SocketAddr {async::SocketAddrV4 {{47, 243, 247, 59}, 80}});
                if (!stream) {
                  std::cout << strerror(int(stream.error())) << std::endl;
                  co_return;
                }
                auto writableBuf = std::array<uint8_t, 1024> {};
                auto buf = std::string_view("GET / HTTP/1.1\r\n\r\n");
                auto readn = co_await stream->send(std::as_bytes(std::span(buf)));
                if (readn) {
                  std::cout << "send " << readn.value() << " bytes" << std::endl;
                } else {
                }
                auto sendn = co_await stream->recv(std::as_writable_bytes(std::span(writableBuf)));
                if (sendn) {
                  std::cout << "recv " << sendn.value() << " bytes" << std::endl;
                } else {
                }
                co_return;
              }(i),
              gReactor);
        }
        co_return;
      }(),
      gReactor);
}
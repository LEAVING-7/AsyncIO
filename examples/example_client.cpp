#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/TcpStream.hpp>
static auto gExecutor = async::MultiThreadExecutor(4);
static auto gReactor = async::Reactor();

int main()
{
  gExecutor.block(
      []() -> Task<> {
        for (int i = 0; i < 1000; i++) {
          auto stream = co_await async::TcpStream::Connect(
              gReactor, async::SocketAddr {async::SocketAddrV4 {{127, 0, 0, 1}, 2333}});
          gExecutor.spawnDetach(
              [](async::TcpStream stream) -> Task<> {
                auto writableBuf = std::array<uint8_t, 1024> {};
                auto buf = std::string_view("GET / HTTP/1.1\r\n\r\n");
                auto readn = co_await stream.send(std::as_bytes(std::span(buf)));
                if (readn) {
                  std::cout << "send " << readn.value() << " bytes" << std::endl;
                } else {
                  std::cout << "readn error" << readn.error().message() << std::endl;
                }
                auto sendn = co_await stream.recv(std::as_writable_bytes(std::span(writableBuf)));
                if (sendn) {
                  std::cout << "recv " << sendn.value() << " bytes" << std::endl;
                } else {
                  std::cout << "sendn error" << sendn.error().message() << std::endl;
                }
                co_return;
              }(std::move(stream.value())),
              gReactor);
        }
      }(),
      gReactor);
}
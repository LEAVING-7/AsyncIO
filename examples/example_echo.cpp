#include <Async/Executor.hpp>
#include <Async/TcpListener.hpp>
#include <concepts>
#include <cstddef>
#include <optional>


int main()
{
  auto reactor = async::Reactor();
  auto executor = async::InlineExecutor();
  auto listener = async::TcpListener::Bind(reactor, async::SocketAddr {async::SocketAddrV4 {{127, 0, 0, 1}, 8080}});
  if (!listener) {
    std::cout << listener.error().message() << std::endl;
    return 1;
  }
  auto result = executor.block(
      [](async::InlineExecutor& e, async::Reactor& r, async::TcpListener listener) -> Task<int> {
        for (int i = 0; i < 3; i++) {
          auto stream = co_await listener.accept(nullptr);
          if (!stream) {
            std::cout << stream.error().message() << std::endl;
            co_return 1;
          } else {
            std::cout << "new connection" << std::endl;
            auto buf = "hi there\n";
            e.spawnDetach(
                [](async::Reactor& r, char const* msg, async::TcpStream stream) -> Task<> {
                  auto writableBuf = std::array<uint8_t, 1024> {};
                  auto readn = co_await stream.recv(std::as_writable_bytes(std::span(writableBuf)));
                  assert(readn);
                  auto buf = std::string_view(msg);
                  auto writen = co_await stream.send(std::as_bytes(std::span(writableBuf)));
                  assert(writen);
                  co_return;
                }(r, buf, std::move(stream.value())),
                r);
          }
        }
        co_return 1234;
      }(executor, reactor, std::move(listener.value())),
      reactor);
  std::cout << result << std::endl;
  return 0;
}
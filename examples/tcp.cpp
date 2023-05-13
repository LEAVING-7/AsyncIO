#include <Async/Executor.hpp>
#include <Async/TcpListener.hpp>
#include <concepts>
#include <optional>
int main()
{
  auto reactor = async::Reactor();
  auto executor = async::MutilThreadExecutor(4);

  auto listener = async::TcpListener::Bind(reactor, async::SocketAddr {async::SocketAddrV4 {{127, 0, 0, 1}, 8080}});
  if (!listener) {
    std::cout << listener.error().message() << std::endl;
    return 1;
  }
  auto result = executor.block(
      [](async::Reactor& r, async::TcpListener listener) -> Task<int> {
        for (int i = 0; i < 3; i++) {
          auto stream = co_await listener.accept(nullptr);
          if (!stream) {
            std::cout << stream.error().message() << std::endl;
            co_return 1;
          }
          std::cout << "accept" << std::endl;
        }
        co_return 1234;
      }(reactor, std::move(listener.value())),
      reactor);
  std::cout << result << std::endl;
  return 0;
}
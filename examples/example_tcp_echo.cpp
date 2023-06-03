#include <Async/Executor.hpp>
#include <Async/TcpListener.hpp>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <optional>
using namespace std::literals;
int main()
{
  using RT = async::Runtime<async::MultiThreadExecutor>;
  RT::Init(4);

  auto listener = async::TcpListener::Bind(RT::GetReactor(), {async::SocketAddrV4::Localhost(8080)});
  if (!listener) {
    return 1;
  }
  auto result = RT::Block([](async::TcpListener listener) -> async::Task<int> {
    while (true) {
      auto stream = co_await listener.accept(nullptr);
      if (!stream) {
        co_return 1;
      } else {
        RT::SpawnDetach([](async::TcpStream stream) -> async::Task<> {
          auto writableBuf = std::array<uint8_t, 1024> {};
          auto readn = co_await stream.recv(std::as_writable_bytes(std::span(writableBuf)));
          assert(readn);
          auto writen = co_await stream.send(std::as_bytes(std::span(writableBuf)));
          assert(writen);
          co_return;
        }(std::move(stream).value()));
      }
    }
    co_return 1234;
  }(std::move(listener).value()));
  std::cout << result << std::endl;
  return 0;
}
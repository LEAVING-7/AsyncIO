#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/TcpListener.hpp>
#include <cstring>
#include <iostream>
using namespace std::literals;
int main()
{
  using RT = async::Runtime<async::InlineExecutor>;
  RT::Init();
  auto listener = async::TcpListener::Bind(RT::GetReactor(), async::SocketAddrV4::Localhost(8080));
  if (!listener) {
    return 1;
  } else {
    RT::Block([](async::TcpListener listener) -> async::Task<> {
      while (true) {
        auto stream = co_await listener.accept(nullptr);
        if (!stream) {
          std::cout << strerror(int(stream.error())) << std::endl;
          co_return;
        } else {
          RT::SpawnDetach([](async::TcpStream stream) -> async::Task<> {
            auto writableBuf = std::array<uint8_t, 1024> {};
            auto readn = co_await stream.recv(std::as_writable_bytes(std::span(writableBuf)));
            assert(readn);
            auto buf = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World\n"sv;
            auto writen = co_await stream.send(std::as_bytes(std::span(buf)));
            assert(writen);
            co_return;
          }(std::move(stream.value())));
        }
      }
      co_return;
    }(std::move(listener.value())));
  }
}
#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/SslStream.hpp>
#include <Async/TlsContext.hpp>
#include <csignal>
#include <iostream>
using namespace std::literals;
int main()
{
  // address sanitizer will report memory leak if multi-thread executor is static, but why?
  using RT = async::Runtime<async::MultiThreadExecutor>;
  RT::Init(4);

  RT::Block([]() -> async::Task<> {
    auto sslCtx = async::TlsContext::Create();
    auto stream = co_await async::SslStream::Connect(sslCtx.value(), RT::GetReactor(),
                                                     {async::SocketAddrV4 {{93, 184, 216, 34}, 443}});
    std::cout << "after connect\n";
    if (!stream) {
      std::cout << "connect error " << stream.error().message() << '\n';
      co_return;
    }
    constexpr auto httpRequest = "POST / HTTP/1.1\r\nHost: www.example.com\r\n\r\n"sv;
    auto sendn = co_await stream->sendAll(std::as_bytes(std::span(httpRequest)));
    if (!sendn) {
      std::cout << "send error " << sendn.error().message() << '\n';
      co_return;
    } else {
      std::cout << "send " << sendn.value() << " bytes" << std::endl;
    }
    auto writableBuf = std::array<uint8_t, 1024> {};
    auto span = std::as_writable_bytes(std::span(writableBuf));
    auto recv = co_await stream->recvAll(span);
    if (!recv) {
      std::cout << "recv error " << recv.error().message() << '\n';
      co_return;
    } else {
      std::cout << std::string_view(reinterpret_cast<char*>(writableBuf.data()), recv.value()) << std::endl;
    }
    while (true) {
      recv = co_await stream->recv(span);
      if (!recv) {
        std::cout << "recv error " << recv.error().message() << '\n';
        co_return;
      } else if (recv.value() == std::size(writableBuf)) {
        // read full, may have more
        std::cout << std::string_view(reinterpret_cast<char*>(writableBuf.data()), recv.value()) << std::endl;
        continue;
      } else {
        std::cout << std::string_view(reinterpret_cast<char*>(writableBuf.data()), recv.value()) << std::endl;
        break;
      }
    }
    auto result = stream->defaultShutdown();
    assert(result);
    co_return;
  }());
}

#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/SslListener.hpp>
#include <Async/TlsContext.hpp>
#include <filesystem>
#include <iostream>
using namespace std::literals;
int main()
{
  using RT = async::Runtime<async::MultiThreadExecutor>;
  RT::Init(4);
  auto sslCtx = async::TlsContext::Create().value();
  sslCtx.use("fullchain.pem", async::TlsContext::FileType::Pem, "privkey.pem", async::TlsContext::FileType::Pem)
      .or_else([](auto e) {
        std::cout << "load cert failed: " << e.message() << '\n';
        std::exit(1);
      });
  puts("cert and key loaded");
  RT::Block([](async::TlsContext& ctx) -> async::Task<> {
    auto listener = async::SslListener::Bind(ctx, RT::GetReactor(), async::SocketAddrV4::Any(11451));
    for (int i = 0; i < 1000; i++) {
      auto conn = co_await listener->accept(ctx, nullptr);
      if (!conn) {
        std::cout << "accept error: " << conn.error().message() << '\n';
        continue;
      }
      auto stream = std::move(conn).value();
      auto buf = std::array<uint8_t, 1024> {};
      auto span = std::as_writable_bytes(std::span(buf));
      auto recv = co_await stream.recv(span);
      if (!recv) {
        std::cout << "recv error: " << recv.error().message() << '\n';
        co_return;
      } else {
        std::cout << std::string_view(reinterpret_cast<char*>(buf.data()), recv.value()) << std::endl;
      }
      constexpr auto httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nfuck"sv;
      auto send = co_await stream.send(std::as_bytes(std::span(httpResponse)));
      if (!send) {
        std::cout << "send error: " << send.error().message() << '\n';
        co_return;
      }
      auto result = stream.defaultShutdown();
      assert(result);
    }
    co_return;
  }(sslCtx));
}
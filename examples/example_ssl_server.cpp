#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/SslListener.hpp>
#include <Async/TlsContext.hpp>
using namespace std::literals;

int main()
{
  auto sslCtx = async::TlsContext::Create().value();
  sslCtx.use("fullchain.pem", async::TlsContext::FileType::Pem, "privkey.pem", async::TlsContext::FileType::Pem)
      .or_else([](auto e) {
        std::exit(1);
        std::cout << "load cert failed: " << e.message() << '\n';
      });
  puts("cert and key loaded");
  auto e = async::MultiThreadExecutor(4);
  auto r = async::Reactor();
  using enum async::TlsContext::FileType;
  e.block(
      [](async::TlsContext& ctx, async::Reactor& r) -> async::Task<> {
        auto listener = async::SslListener::Bind(ctx, r, async::SocketAddrV4::Any(11451));
        auto conn = co_await listener.accept(ctx, nullptr);
        if (!conn) {
          std::cout << "accept error: " << conn.error().message() << '\n';
          co_return;
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
        constexpr auto httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!"sv;
        auto send = co_await stream.send(std::as_bytes(std::span(httpResponse)));
        if (!send) {
          std::cout << "send error: " << send.error().message() << '\n';
          co_return;
        }
        assert(listener.defaultShutdown());
        co_return;
      }(sslCtx, r),
      r);
}
#include <Async/Executor.hpp>
#include <Async/Reactor.hpp>
#include <Async/TcpListener.hpp>

static auto gExecutor = async::MultiThreadExecutor(8);
static auto gReactor = async::Reactor();

int main()
{
  auto listener = async::TcpListener::Bind(gReactor, async::SocketAddr {async::SocketAddrV4 {{0, 0, 0, 0}, 2333}});
  if (!listener) {
    std::cout << listener.error().message() << std::endl;
    return 1;
  } else {
    gExecutor.block(
        [](async::TcpListener listener) -> Task<> {
          for (int i = 0;; i++) {
            auto stream = co_await listener.accept(nullptr);
            if (!stream) {
              std::cout << stream.error().message() << std::endl;
              co_return;
            } else {
              std::cout << "new connection at " << stream->getSocket().raw() << std::endl;
              auto buf = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World\n";
              gExecutor.spawnDetach(
                  [](char const* msg, async::TcpStream stream) -> Task<> {
                    auto writableBuf = std::array<uint8_t, 1024> {};
                    auto readn = co_await stream.read(std::as_writable_bytes(std::span(writableBuf)));
                    assert(readn);
                    auto buf = std::string_view(msg);
                    auto writen = co_await stream.write(std::as_bytes(std::span(buf)));
                    assert(writen);
                    co_return;
                  }(buf, std::move(stream.value())),
                  gReactor);
            }
          }
          co_return;
        }(std::move(listener.value())),
        gReactor);
  }
}
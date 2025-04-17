#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../macros/coop-assert.hpp"
#include "server.hpp"

namespace net::tcp {
auto TCPServerBackend::post_accept(sock::Socket& client) -> bool {
    ensure(client.set_sockopt(SO_KEEPALIVE, 1));
    ensure(client.set_sockopt(IPPROTO_TCP, TCP_KEEPIDLE, 20));
    ensure(client.set_sockopt(IPPROTO_TCP, TCP_KEEPINTVL, 5));
    ensure(client.set_sockopt(IPPROTO_TCP, TCP_KEEPCNT, 2));
    return true;
}

auto TCPServerBackend::start(const uint16_t port, const int backlog) -> coop::Async<bool> {
    coop_ensure(sock::init_socket_system());
    sock = sock::Socket(socket(AF_INET, SOCK_STREAM, 0));
    coop_ensure(sock.is_valid());

#if !defined(__WIN32__)
    auto value = 1;
    coop_ensure(setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value)) == 0);
#endif

    auto info            = sockaddr_in();
    info.sin_family      = AF_INET;
    info.sin_port        = htons(port);
    info.sin_addr.s_addr = INADDR_ANY;
    coop_ensure(bind(sock.fd, (sockaddr*)&info, sizeof(info)) == 0);
    coop_ensure(listen(sock.fd, backlog) == 0);

    (co_await coop::reveal_runner())->push_task(task_main(), &task);

    co_return true;
}
} // namespace net::tcp

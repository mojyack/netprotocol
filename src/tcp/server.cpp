#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../macros/coop-assert.hpp"
#include "common.hpp"
#include "server.hpp"

namespace net::tcp {
auto TCPServerBackend::post_accept(sock::Socket& client) -> bool {
    ensure(set_keepalive(client, 20, 5, 2));
    return true;
}

auto TCPServerBackend::get_peer_addr_ipv4(const net::ClientData& client) -> std::optional<uint32_t> {
    const auto& sock = std::bit_cast<net::sock::SocketClientData*>(&client)->sock;
    return ::net::tcp::get_peer_addr_ipv4(sock.fd);
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

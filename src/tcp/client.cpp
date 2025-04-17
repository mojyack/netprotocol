#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/thread.hpp>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../macros/autoptr.hpp"
#include "../macros/coop-assert.hpp"
#include "client.hpp"

namespace {
declare_autoptr(AddrInfo, addrinfo, freeaddrinfo);
}

namespace net::tcp {
auto TCPClientBackend::connect(const char* const host, const uint16_t port) -> coop::Async<bool> {
    coop_ensure(sock::init_socket_system());

    auto result = AutoAddrInfo();
    {
        auto hint        = addrinfo();
        hint.ai_family   = AF_UNSPEC;
        hint.ai_socktype = SOCK_STREAM;
        coop_ensure(co_await coop::run_blocking([&] { return getaddrinfo(host, NULL, &hint, std::inout_ptr(result)); }) == 0);
    }
    for(auto ptr = result.get(); ptr != NULL; ptr = ptr->ai_next) {
        // PRINT("family={} socktype={} protocol={}", ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        auto sock = sock::Socket(socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol));
        if(!sock.is_valid()) {
            continue;
        }
        coop_ensure(sock.set_blocking(false));
        ((sockaddr_in*)ptr->ai_addr)->sin_port = htons(port); // override port
        if(const auto r = ::connect(sock.fd, ptr->ai_addr, ptr->ai_addrlen); !(r == 0 || (r == -1 && errno == EINPROGRESS))) {
            continue;
        }
        if((co_await coop::wait_for_file(sock.fd, false, true)).error || sock.get_sockopt(SO_ERROR) != 0) {
            continue;
        }
        this->sock = std::move(sock);
        break;
    }
    coop_ensure(this->sock.is_valid());

    (co_await coop::reveal_runner())->push_task(task_main(), &task);

    co_return true;
}

auto TCPClientBackend::connect(const std::array<uint8_t, 4> addr, const uint16_t port) -> coop::Async<bool> {
    coop_ensure(sock::init_socket_system());
    sock = sock::Socket(socket(AF_INET, SOCK_STREAM, 0));
    coop_ensure(sock.is_valid());

    auto info            = sockaddr_in();
    info.sin_family      = AF_INET;
    info.sin_port        = htons(port);
    info.sin_addr.s_addr = htonl(sock::build_ipv4_addr(addr[0], addr[1], addr[2], addr[3]));
    coop_ensure(sock.set_blocking(false));
    const auto status = ::connect(sock.fd, (sockaddr*)&info, sizeof(info));
    coop_ensure(status == 0 || (status == -1 && errno == EINPROGRESS));
    coop_ensure(!(co_await coop::wait_for_file(sock.fd, false, true)).error);
    coop_ensure(sock.get_sockopt(SO_ERROR) == 0);
    coop_ensure(sock.set_sockopt(SO_KEEPALIVE, 1));
    coop_ensure(sock.set_sockopt(IPPROTO_TCP, TCP_KEEPIDLE, 20));
    coop_ensure(sock.set_sockopt(IPPROTO_TCP, TCP_KEEPCNT, 1));

    (co_await coop::reveal_runner())->push_task(task_main(), &task);

    co_return true;
}
} // namespace net::tcp

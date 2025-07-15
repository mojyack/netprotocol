#include <cstring>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "../macros/assert.hpp"
#include "common.hpp"

namespace net::tcp {
auto set_keepalive(sock::Socket& sock, const int idle, const int interval, const int count) -> bool {
    ensure(sock.set_sockopt(SO_KEEPALIVE, 1));
    ensure(sock.set_sockopt(IPPROTO_TCP, TCP_KEEPIDLE, idle));
    ensure(sock.set_sockopt(IPPROTO_TCP, TCP_KEEPINTVL, interval));
    ensure(sock.set_sockopt(IPPROTO_TCP, TCP_KEEPCNT, count));
    return true;
}

auto get_peer_addr_ipv4(const int fd) -> std::optional<uint32_t> {
    auto addr = sockaddr_storage();
    auto len  = socklen_t(sizeof(addr));
    ensure(getpeername(fd, (sockaddr*)&addr, &len) == 0, "errno={}({})", errno, strerror(errno));
    if(addr.ss_family == AF_INET) {
        return ntohl(((sockaddr_in*)&addr)->sin_addr.s_addr);
    } else {
        bail("unimplemented address family {}", addr.ss_family);
    }
}
} // namespace net::tcp

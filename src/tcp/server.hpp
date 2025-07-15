#pragma once
#include "../socket/server.hpp"

namespace net::tcp {
struct TCPServerBackend : sock::SocketServerBackend {
    auto post_accept(sock::Socket& client) -> bool override;

    // backend-specific
    static auto get_peer_addr_ipv4(const net::ClientData& client) -> std::optional<uint32_t>;

    auto start(uint16_t port, int backlog = 3) -> coop::Async<bool>;
};
} // namespace net::tcp

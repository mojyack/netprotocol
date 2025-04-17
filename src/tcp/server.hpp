#pragma once
#include "../socket/server.hpp"

namespace net::tcp {
struct TCPServerBackend : sock::SocketServerBackend {
    auto post_accept(sock::Socket& client) -> bool override;

    // backend-specific
    auto start(uint16_t port, int backlog = 3) -> coop::Async<bool>;
};
} // namespace net::tcp

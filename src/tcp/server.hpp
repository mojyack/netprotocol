#pragma once
#include "../socket/server.hpp"

namespace net::tcp {
struct TCPServerBackend : sock::SocketServerBackend {
    // backend-specific
    auto start(uint16_t port, int backlog = 3) -> coop::Async<bool>;
};
} // namespace net::tcp

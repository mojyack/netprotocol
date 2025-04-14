#pragma once
#include "../socket/server.hpp"

namespace net::uds {
struct UDSServerBackend : sock::SocketServerBackend {
    // private
    std::string path;

    // backend-specific
    auto start(std::string_view path, int backlog = 3) -> coop::Async<bool>;

    ~UDSServerBackend();
};
} // namespace net::uds

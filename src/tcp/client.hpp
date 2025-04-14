#pragma once
#include "../socket/client.hpp"

namespace net::tcp {
struct TCPClientBackend : sock::SocketClientBackend {
    auto connect(const char* host, uint16_t port) -> coop::Async<bool>;
    auto connect(std::array<uint8_t, 4> addr, uint16_t port) -> coop::Async<bool>;
};
} // namespace net::tcp

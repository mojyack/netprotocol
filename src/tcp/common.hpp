#pragma once
#include <optional>

#include "../socket/socket.hpp"

namespace net::tcp {
auto set_keepalive(sock::Socket& sock, int idle, int interval, int count) -> bool;
auto get_peer_addr_ipv4(int fd) -> std::optional<uint32_t>;
} // namespace net::tcp

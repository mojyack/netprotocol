#pragma once
#include <optional>

namespace net::tcp {
auto get_peer_addr_ipv4(int fd) -> std::optional<uint32_t>;
}

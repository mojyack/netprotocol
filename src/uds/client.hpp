#pragma once
#include "../socket/client.hpp"

namespace net::uds {
struct UDSClientBackend : sock::SocketClientBackend {
    auto connect(std::string_view path) -> coop::Async<bool>;
};
} // namespace net::uds

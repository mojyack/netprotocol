#pragma once
#include <coop/generator.hpp>
#include <coop/runner-pre.hpp>

#include "../backend.hpp"
#include "socket.hpp"

namespace net::sock {
struct SocketClientBackend : ClientBackend {
    // private
    coop::TaskHandle task;
    sock::Socket     sock;

    auto task_main() -> coop::Async<void>;

    // overrides
    auto send(PrependableBuffer data) -> coop::Async<bool> override;
    auto finish() -> coop::Async<bool> override;

    ~SocketClientBackend();
};
} // namespace net::sock

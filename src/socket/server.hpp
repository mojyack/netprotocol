#pragma once
#include <coop/generator.hpp>
#include <coop/runner.hpp>

#include "../backend.hpp"
#include "socket.hpp"

namespace net::sock {
struct SocketClientData : ClientData {
    coop::TaskHandle task;
    sock::Socket     sock;
    bool             running = true;
};

struct SocketServerBackend : ServerBackend {
    // private
    coop::TaskHandle            task;
    sock::Socket                sock;
    std::list<SocketClientData> client_data;

    auto task_main() -> coop::Async<void>;
    auto client_main(decltype(client_data)::iterator iter) -> coop::Async<void>;

    // overrides
    auto shutdown() -> coop::Async<bool> override;
    auto disconnect(const ClientData& client) -> coop::Async<bool> override;
    auto send(const ClientData& client, BytesRef data) -> coop::Async<bool> override;

    ~SocketServerBackend();
};
} // namespace net::sock

#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/timer.hpp>

#include "net/uds/client.hpp"
#include "net/uds/server.hpp"

using Server = net::uds::UDSServerBackend;
using Client = net::uds::UDSClientBackend;

auto start_server_backend(net::ServerBackend& server) -> coop::Async<bool> {
    co_return co_await std::bit_cast<Server*>(&server)->start("/tmp/netprotocol.sock");
}

auto start_client_backend(net::ClientBackend& client) -> coop::Async<bool> {
    co_return co_await std::bit_cast<Client*>(&client)->connect("/tmp/netprotocol.sock");
}

#include "common.cpp"

auto main() -> int {
    auto runner = coop::Runner();
    runner.push_task(test());
    runner.run();
    if(pass) {
        std::println("pass");
        return 0;
    } else {
        return -1;
    }
}

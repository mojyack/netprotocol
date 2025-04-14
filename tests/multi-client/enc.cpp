#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/timer.hpp>

#include "net/enc/client.hpp"
#include "net/enc/server.hpp"
#include "net/tcp/client.hpp"
#include "net/tcp/server.hpp"

using Server  = net::enc::ServerBackendEncAdaptor;
using Client  = net::enc::ClientBackendEncAdaptor;
using IServer = net::tcp::TCPServerBackend;
using IClient = net::tcp::TCPClientBackend;

auto start_server_backend(net::ServerBackend& server) -> coop::Async<bool> {
    return std::bit_cast<Server*>(&server)->start(new IServer(), 8080);
}

auto start_client_backend(net::ClientBackend& client) -> coop::Async<bool> {
    return std::bit_cast<Client*>(&client)->connect(new IClient(), std::array<uint8_t, 4>{127, 0, 0, 1}, 8080);
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

#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/timer.hpp>

#include "net/tcp/client.hpp"
#include "net/tcp/server.hpp"

using Server = net::tcp::TCPServerBackend;
using Client = net::tcp::TCPClientBackend;

auto by_hostname = false;

auto start_server_backend(net::ServerBackend& server) -> coop::Async<bool> {
    co_return co_await std::bit_cast<Server*>(&server)->start(8080);
}

auto start_client_backend(net::ClientBackend& client) -> coop::Async<bool> {
    auto& c = *std::bit_cast<Client*>(&client);
    co_return co_await (by_hostname ? c.connect("localhost", 8080) : c.connect({127, 0, 0, 1}, 8080));
}

#include "common.cpp"

auto main() -> int {
    auto runner = coop::Runner();

    by_hostname = false;
    runner.push_task(test());
    runner.run();
    const auto pass_1 = std::exchange(pass, false);

    by_hostname = true;
    runner.push_task(test());
    runner.run();
    const auto pass_2 = std::exchange(pass, false);

    if(pass_1 && pass_2) {
        std::println("pass");
        return 0;
    } else {
        return -1;
    }
}

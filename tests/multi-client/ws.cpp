#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/timer.hpp>

#include "net/ws/client.hpp"
#include "net/ws/server.hpp"
#include "net/ws/ws/misc.hpp"

using Server = net::ws::WebSocketServerBackend;
using Client = net::ws::WebSocketClientBackend;

auto runner   = coop::Runner();
auto injector = coop::TaskInjector(runner);

auto start_server_backend(net::ServerBackend& server) -> coop::Async<bool> {
    co_return co_await std::bit_cast<Server*>(&server)->start(
        {
            .protocol = "message",
            .port     = 8080,
            // .cert        = "files/localhost.cert",
            // .private_key = "files/localhost.key",
        },
        injector);
}

auto start_client_backend(net::ClientBackend& client) -> coop::Async<bool> {
    co_return co_await std::bit_cast<Client*>(&client)->connect(
        {
            .address   = "localhost",
            .path      = "/",
            .protocol  = "message",
            .port      = 8080,
            .ssl_level = ws::client::SSLLevel::Disable,
            // .cert = "files/localhost.cert",
        },
        injector);
}

#include "common.cpp"

auto test2() -> coop::Async<void> {
    co_await test();
    injector.blocker.stop();
}

auto main() -> int {
    ws::set_log_level(1);
    runner.push_task(test2());
    runner.run();
    if(pass) {
        std::println("pass");
        return 0;
    } else {
        return -1;
    }
}

#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/timer.hpp>

#include "../macros/coop-assert.hpp"
#include "../util/charconv.hpp"
#include "../util/span.hpp"
#include "cases.hpp"

auto num_clients = 0;
struct ClientData {
    int num;
};

auto multiple_clients_test(net::ServerBackend& server, std::vector<net::ClientBackend*> clients) -> coop::Async<bool> {
    server.alloc_client = [](net::ClientData& client) -> coop::Async<void> { client.data = new ClientData{num_clients += 1};co_return; };
    server.free_client  = [](void* ptr) -> coop::Async<void> { delete(ClientData*)(ptr); co_return; };
    server.on_received  = [&server](const net::ClientData& client, net::BytesRef data) -> coop::Async<void> {
        auto& c = *(ClientData*)client.data;
        std::println("received message from client {}: {}", c.num, from_span(data));
        coop_ensure(co_await server.send(client, to_span(std::format("{}", data.size()))));
        co_return;
    };
    coop_ensure(co_await start_server_backend(server));
    std::println("server started");

    auto count  = 0uz;
    auto closed = 0uz;
    for(const auto client : clients) {
        client->on_received = [&count](net::BytesRef data) -> coop::Async<void> {
            std::println("received message from server: {}", from_span(data));
            if(const auto resp = from_chars<size_t>(from_span(data))) {
                count += *resp;
            }
            co_return;
        };
        client->on_closed = [&closed]() { closed += 1; };
        coop_ensure(co_await start_client_backend(*client));
        std::println("client connected");
    }

    const auto message = to_span("hello");
    for(auto i = 0; i < 3; i += 1) {
        for(auto& client : clients) {
            coop_ensure(co_await client->send(message));
        }
    }
    co_await coop::sleep(std::chrono::milliseconds(100));
    coop_ensure(count == clients.size() * message.size() * 3, "{}", count);

    for(auto& client : clients) {
        co_await client->finish();
    }
    co_await server.shutdown();
    coop_ensure(closed == clients.size(), "{}", closed);

    co_return true;
}

auto closed_callback_test(net::ServerBackend& server, std::vector<net::ClientBackend*> clients) -> coop::Async<bool> {
    coop_ensure(co_await start_server_backend(server));
    std::println("server started");

    auto closed = 0uz;
    for(const auto client : clients) {
        client->on_closed = [&closed]() { closed += 1; };
        coop_ensure(co_await start_client_backend(*client));
        std::println("client connected");
    }

    co_await server.shutdown();
    co_await coop::sleep(std::chrono::milliseconds(100));
    coop_ensure(closed == clients.size(), "{}", closed);
    for(auto& client : clients) {
        co_await client->finish();
    }

    co_return true;
}

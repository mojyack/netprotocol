#include <ranges>

#include <coop/generator.hpp>
#include <coop/parallel.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/single-event.hpp>
#include <coop/thread-event.hpp>
#include <coop/timer.hpp>

#include "../macros/coop-unwrap.hpp"
#include "../net/packet-parser.hpp"
#include "cases.hpp"

namespace {
// protocol
struct PacketType {
    enum : uint8_t {
        Hello = 0,
        HelloReply,
        RawPayload,
    };
};

struct Hello {
    constexpr static auto pt = PacketType::Hello;

    SerdeFieldsBegin;
    std::string SerdeField(name);
    SerdeFieldsEnd;
};

struct HelloReply {
    constexpr static auto pt = PacketType::HelloReply;

    SerdeFieldsBegin;
    std::string SerdeField(message);
    SerdeFieldsEnd;
};

// server
struct ClientData {
    net::PacketParser parser;

    auto handle_payload(const net::Header header, const net::BytesRef payload) -> coop::Async<bool> {
        switch(header.type) {
        case PacketType::Hello: {
            coop_unwrap(request, (serde::load<net::BinaryFormat, Hello>(payload)));
            coop_ensure(co_await parser.send_packet(HelloReply{.message = std::format("hello, {}!", request.name)}, header.id));
            break;
        }
        case PacketType::RawPayload: {
            const auto size = payload.size();
            coop_ensure(co_await parser.send_packet(PacketType::RawPayload, &size, sizeof(size), header.id));
            break;
        }
        default:
            coop_bail("unhandled packet type {}", header.id);
            break;
        }
        co_return true;
    };

    ClientData(net::ServerBackend& backend, net::ClientData& client) {
        parser.send_data = [&backend, &client](const net::BytesRef payload) -> coop::Async<bool> {
            constexpr auto error_value = false;
            co_ensure_v(co_await backend.send(client, payload));
            co_return true;
        };
    }
};

auto start_server(net::ServerBackend& backend) -> coop::Async<bool> {
    backend.alloc_client = [&backend](net::ClientData& client) -> coop::Async<void> { client.data = new ClientData(backend, client); co_return; };
    backend.free_client  = [](void* ptr) -> coop::Async<void> { delete(ClientData*)(ptr);co_return; };
    backend.on_received  = [](const net::ClientData& client, net::BytesRef data) -> coop::Async<void> {
        auto& c = *(ClientData*)client.data;
        if(const auto p = c.parser.parse_received(data)) {
            coop_ensure(co_await c.handle_payload(p->header, p->payload));
        }
        co_return;
    };
    coop_ensure(co_await start_server_backend(backend));
    co_return true;
}

// client
auto start_client(net::ClientBackend& backend) -> coop::Async<bool> {
    auto parser           = net::PacketParser();
    auto raw_payload_size = 0uz;
    parser.send_data      = [&backend](const net::BytesRef payload) -> coop::Async<bool> {
        constexpr auto error_value = false;
        co_ensure_v(co_await backend.send(payload));
        co_return true;
    };
    backend.on_received = [&parser, &raw_payload_size](net::BytesRef data) -> coop::Async<void> {
        if(const auto p = parser.parse_received(data)) {
            const auto [header, payload] = *p;
            if(header.type == PacketType::RawPayload) {
                coop_ensure(payload.size() == sizeof(size_t));
                raw_payload_size += *std::bit_cast<size_t*>(payload.data());
            } else {
                coop_ensure(co_await parser.callbacks.invoke(header, payload));
            }
        }
        co_return;
    };

    constexpr auto error_value = false;
    coop_ensure(co_await start_client_backend(backend));

    const auto name = std::format("client-{:p}", &parser);
    for(auto i = 0; i < 3; i += 1) {
        const auto message = std::format("{} {}", name, i);
        co_unwrap_v(response, co_await parser.receive_response<HelloReply>(Hello{.name = message}));
        std::println("received reply {}", response.message);
        coop_ensure(response.message == std::format("hello, {}!", message));
        co_await coop::sleep(std::chrono::milliseconds(100));
    }
    for(auto i = 0; i < 3; i += 1) {
        const auto payload = std::vector<std::byte>(i * 100);
        coop_ensure(co_await parser.send_packet(PacketType::RawPayload, payload.data(), payload.size()));
        co_await coop::sleep(std::chrono::milliseconds(100));
        coop_ensure(raw_payload_size == payload.size());
        raw_payload_size = 0;
    }

    co_return true;
}
} // namespace

auto simple_server_test(net::ServerBackend& server, std::vector<net::ClientBackend*> clients) -> coop::Async<bool> {
    coop_ensure(co_await start_server(server));
    auto tasks = std::vector<coop::Async<bool>>(clients.size());
    for(auto&& [task, client] : std::views::zip(tasks, clients)) {
        task = start_client(*client);
    }
    for(const auto result : co_await coop::run_vec(std::move(tasks))) {
        coop_ensure(result);
    }
    co_return true;
}

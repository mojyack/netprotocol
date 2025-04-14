#include "net/mock-c2c/client.hpp"

namespace {
using Client = net::mock::MockClient;

auto client_1 = (Client*)(nullptr);

auto prepare_client_1(Client&) -> void {
}

auto start_client_1(Client& client) -> coop::Async<bool> {
    client_1 = &client;
    return client.connect();
}

auto prepare_client_2(Client&) -> void {
}

auto start_client_2(Client& client) -> coop::Async<bool> {
    return client.connect(*client_1);
}
} // namespace

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

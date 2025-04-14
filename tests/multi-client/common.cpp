#include <ranges>

#include "cases/cases.hpp"
#include "macros/coop-assert.hpp"
#include "net/backend.hpp"

template <class T, size_t N>
auto make_ptr_vec(std::array<T, N>& clients) -> std::vector<net::ClientBackend*> {
    auto ret = std::vector<net::ClientBackend*>(N);
    for(auto&& [i, p] : std::views::zip(clients, ret)) {
        p = &i;
    }
    return ret;
}

auto pass = false;

auto test() -> coop::Async<void> {
    std::println("==== backend multiple client test ====");
    {
        auto server  = Server();
        auto clients = std::array<Client, 4>();
        coop_ensure(co_await multiple_clients_test(server, make_ptr_vec(clients)));
    }
    std::println("==== backend close test ====");
    {
        auto server  = Server();
        auto clients = std::array<Client, 4>();
        coop_ensure(co_await closed_callback_test(server, make_ptr_vec(clients)));
    }
    std::println("==== simple server test ====");
    {
        auto server  = Server();
        auto clients = std::array<Client, 4>();
        coop_ensure(co_await simple_server_test(server, make_ptr_vec(clients)));
    }
    pass = true;
}

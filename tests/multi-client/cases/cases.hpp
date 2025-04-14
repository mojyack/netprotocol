#pragma once
#include "../net/backend.hpp"

// defined in backend sources
auto start_server_backend(net::ServerBackend& server) -> coop::Async<bool>;
auto start_client_backend(net::ClientBackend& client) -> coop::Async<bool>;

auto multiple_clients_test(net::ServerBackend& server, std::vector<net::ClientBackend*> clients) -> coop::Async<bool>;
auto closed_callback_test(net::ServerBackend& server, std::vector<net::ClientBackend*> clients) -> coop::Async<bool>;
auto simple_server_test(net::ServerBackend& server, std::vector<net::ClientBackend*> clients) -> coop::Async<bool>;

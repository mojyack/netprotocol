#pragma once
#include <coop/single-event.hpp>

#include "../backend.hpp"

namespace net::mock {
struct MockClient : ClientBackend {
    // private
    coop::TaskHandle        task;
    std::vector<BytesArray> buffer;
    coop::SingleEvent       avaiable;
    MockClient*             peer = nullptr;

    auto task_main() -> coop::Async<void>;

    // overrides
    auto send(BytesRef data) -> coop::Async<bool> override;
    auto finish() -> coop::Async<bool> override;

    auto connect() -> coop::Async<bool>;
    auto connect(MockClient& peer) -> coop::Async<bool>;

    ~MockClient();
};
} // namespace net::mock


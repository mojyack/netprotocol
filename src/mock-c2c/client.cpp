#include <coop/task-handle.hpp>

#include "../macros/coop-assert.hpp"
#include "client.hpp"

namespace net::mock {
auto MockClient::task_main() -> coop::Async<void> {
loop:
    co_await avaiable;
    for(auto& buffer : std::exchange(buffer, {})) {
        co_await on_received(std::move(buffer));
    }
    goto loop;
}

auto MockClient::send(PrependableBuffer data) -> coop::Async<bool> {
    coop_ensure(peer != nullptr);
    peer->buffer.emplace_back(std::move(data));
    peer->avaiable.notify();
    co_return true;
}

auto MockClient::finish() -> coop::Async<bool> {
    task.cancel();
    if(peer == nullptr) {
        co_return true;
    }
    const auto other = std::exchange(peer, nullptr);
    other->peer      = nullptr;
    other->on_closed();
    co_return co_await other->finish();
}

auto MockClient::connect() -> coop::Async<bool> {
    (co_await coop::reveal_runner())->push_task(task_main(), &task);
    co_return true;
}

auto MockClient::connect(MockClient& peer) -> coop::Async<bool> {
    this->peer       = &peer;
    this->peer->peer = this;
    return connect();
}

MockClient::~MockClient() {
    if(task.task != nullptr) {
        task.runner->await(finish());
    }
}
} // namespace net::mock

#include <coop/task-handle.hpp>
#include <coop/thread.hpp>

#include "client.hpp"
#include "macros/coop-assert.hpp"
#include "util/cleaner.hpp"

namespace net::ws {
auto WebSocketClientBackend::task_main() -> coop::Async<void> {
    auto cleaner = Cleaner{[&] { on_closed(); }};
    while(context.state == ::ws::client::State::Connected) {
        co_await coop::run_blocking([this] { context.process(); });
    }
}

auto WebSocketClientBackend::send(PrependableBuffer data) -> coop::Async<bool> {
    coop_ensure(context.send(std::move(data)));
    co_return true;
}

auto WebSocketClientBackend::finish() -> coop::Async<bool> {
    if(context.state == ::ws::client::State::Connected) {
        context.shutdown();
    }
    task.cancel();
    co_return true;
}

auto WebSocketClientBackend::connect(const ::ws::client::ContextParams& params, coop::TaskInjector& injector) -> coop::Async<bool> {
    context.handler = [this, &injector](PrependableBuffer payload) -> void {
        injector.inject_task([this, &payload]() -> coop::Async<void> {
            co_await on_received(std::move(payload));
        }());
    };
    coop_ensure(co_await coop::run_blocking([this, &params] { return context.init(params); }));
    (co_await coop::reveal_runner())->push_task(task_main(), &task);
    co_return true;
}

WebSocketClientBackend::~WebSocketClientBackend() {
    if(task.task != nullptr) {
        task.runner->await(finish());
    }
}
} // namespace net::ws

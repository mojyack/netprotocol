#pragma once
#include <coop/generator.hpp>
#include <coop/runner.hpp>
#include <coop/task-injector.hpp>

#include "../backend.hpp"
#include "ws/client.hpp"

namespace net::ws {
struct WebSocketClientBackend : ClientBackend {
    // private
    ::ws::client::Context context;
    coop::TaskHandle      task;

    auto task_main() -> coop::Async<void>;

    // overrides
    auto send(BytesRef data) -> coop::Async<bool> override;
    auto finish() -> coop::Async<bool> override;

    // backend-specific
    auto connect(const ::ws::client::ContextParams& params, coop::TaskInjector& injector) -> coop::Async<bool>;

    ~WebSocketClientBackend();
};
} // namespace net::ws

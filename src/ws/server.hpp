#pragma once
#include <coop/generator.hpp>
#include <coop/runner.hpp>
#include <coop/task-injector.hpp>

#include "../backend.hpp"
#include "ws/server.hpp"

namespace net::ws {
struct WebSocketServerBackend : ServerBackend {
    // private
    ::ws::server::Context context;
    coop::TaskHandle      task;
    coop::TaskInjector*   injector;

    auto task_main() -> coop::Async<void>;

    // overrides
    auto shutdown() -> coop::Async<bool> override;
    auto disconnect(const ClientData& client) -> coop::Async<bool> override;
    auto send(const ClientData& client, PrependableBuffer data) -> coop::Async<bool> override;

    // backend-specific
    auto start(const ::ws::server::ContextParams& params, coop::TaskInjector& injector) -> coop::Async<bool>;

    ~WebSocketServerBackend();
};
} // namespace net::ws

#include <coop/task-handle.hpp>
#include <coop/thread.hpp>
#include <libwebsockets.h>

#include "macros/coop-assert.hpp"
#include "server.hpp"

namespace net::ws {
namespace {
struct WebSocketClientData : ClientData {
    ::ws::server::Client* client;
};

struct SessionDataInitializer : ::ws::server::SessionDataInitializer {
    WebSocketServerBackend* self;
    std::thread::id         main_thread_id;

    auto alloc(::ws::server::Client* client) -> void* override {
        const auto ptr = new WebSocketClientData();
        ptr->client    = client;
        self->injector->inject_task([this, ptr]() -> coop::Async<void> {
            co_await self->alloc_client(*ptr);
        }());
        return ptr;
    }

    auto free(void* const vptr) -> void override {
        const auto ptr = (WebSocketClientData*)vptr;
        if(std::this_thread::get_id() == main_thread_id) {
            self->task.runner->push_dependent_task(self->free_client(ptr->data));
            delete ptr;
        } else {
            self->injector->inject_task([this, ptr]() -> coop::Async<void> {
                co_await self->free_client(ptr->data);
                delete ptr;
            }());
        }
    }
};
} // namespace

auto WebSocketServerBackend::task_main() -> coop::Async<void> {
    while(context.state == ::ws::server::State::Connected) {
        co_await coop::run_blocking([this] { context.process(); });
    }
}

auto WebSocketServerBackend::shutdown() -> coop::Async<bool> {
    if(context.state == ::ws::server::State::Connected) {
        context.shutdown();
        context.context.reset();
    }
    co_await task.join();
    co_return true;
}

auto WebSocketServerBackend::disconnect(const ClientData& client) -> coop::Async<bool> {
    auto& wsc = *std::bit_cast<WebSocketClientData*>(&client);
    lws_set_timeout(wsc.client, pending_timeout(1), LWS_TO_KILL_ASYNC);
    co_return true;
}

auto WebSocketServerBackend::send(const ClientData& client, PrependableBuffer data) -> coop::Async<bool> {
    auto& wsc = *std::bit_cast<WebSocketClientData*>(&client);
    coop_ensure(context.send(wsc.client, std::move(data)));
    co_return true;
}

auto WebSocketServerBackend::start(const ::ws::server::ContextParams& params, coop::TaskInjector& injector) -> coop::Async<bool> {
    this->injector  = &injector;
    context.handler = [this, &injector](::ws::server::Client* client, PrependableBuffer payload) -> void {
        auto& session = *std::bit_cast<WebSocketClientData*>(::ws::server::client_to_userdata(client));
        injector.inject_task([this, &session, &payload]() -> coop::Async<void> {
            co_await on_received(session, std::move(payload));
        }());
    };
    {
        const auto initer      = new SessionDataInitializer();
        initer->self           = this;
        initer->main_thread_id = std::this_thread::get_id();
        context.session_data_initer.reset(initer);
    }
    coop_ensure(context.init(params));

    (co_await coop::reveal_runner())->push_task(task_main(), &task);

    co_return true;
}

WebSocketServerBackend::~WebSocketServerBackend() {
    if(task.task != nullptr) {
        task.runner->await(shutdown());
    }
}
} // namespace net::ws

#pragma once
#include <functional>

#include <coop/generator.hpp>
#include <coop/promise.hpp>

#include "util/prependable-buffer.hpp"

namespace net {
struct ClientBackend {
    std::function<coop::Async<void>(PrependableBuffer data)> on_received = [](PrependableBuffer) -> coop::Async<void> { co_return; }; // backend to user
    std::function<void()>                                    on_closed   = [] {};

    virtual auto send(PrependableBuffer data) -> coop::Async<bool> = 0; // user to backend
    virtual auto finish() -> coop::Async<bool>                     = 0;

    virtual ~ClientBackend() {}
};

struct ClientData {
    void* data;
};

struct ServerBackend {
    std::function<coop::Async<void>(const ClientData& client, PrependableBuffer data)> on_received  = [](const ClientData&, PrependableBuffer) -> coop::Async<void> { co_return; }; // backend to user
    std::function<coop::Async<void>(ClientData& client)>                               alloc_client = [](ClientData&) -> coop::Async<void> { co_return; };
    std::function<coop::Async<void>(void*)>                                            free_client  = [](void*) -> coop::Async<void> { co_return; };

    virtual auto shutdown() -> coop::Async<bool>                                             = 0;
    virtual auto disconnect(const ClientData& client) -> coop::Async<bool>                   = 0;
    virtual auto send(const ClientData& client, PrependableBuffer data) -> coop::Async<bool> = 0; // user to backend
    virtual ~ServerBackend() {}
};
} // namespace net

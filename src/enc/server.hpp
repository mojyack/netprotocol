#pragma once
#include <coop/generator.hpp>
#include <coop/runner.hpp>

#include "../backend.hpp"
#include "crypto/cipher.hpp"

namespace net::enc {
struct ServerBackendEncAdaptor : ServerBackend {
    // private
    std::unique_ptr<ServerBackend> inner;
    crypto::AutoCipherContext      cipher_context;

    auto setup_inner(ServerBackend* backend) -> void;

    // overrides
    auto shutdown() -> coop::Async<bool> override;
    auto disconnect(const ClientData& client) -> coop::Async<bool> override;
    auto send(const ClientData& client, PrependableBuffer data) -> coop::Async<bool> override;

    // backend-specific
    template <class T, class... Args>
        requires(std::is_convertible_v<T*, ServerBackend*>)
    auto start(T* backend, Args&&... args) -> coop::Async<bool> {
        setup_inner(backend);
        return backend->start(std::forward<Args>(args)...);
    }
};
} // namespace net::enc

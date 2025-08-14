#pragma once
#include "../backend.hpp"
#include "../common.hpp"
#include "crypto/cipher.hpp"

namespace net::enc {
struct ClientBackendEncAdaptor : ClientBackend {
    // private
    std::unique_ptr<ClientBackend> inner;
    BytesArray                     secret;
    crypto::AutoCipherContext      cipher_context;

    auto connect_inner(ClientBackend* backend, coop::Async<bool> start) -> coop::Async<bool>;

    // overrides
    auto send(PrependableBuffer data) -> coop::Async<bool> override;
    auto finish() -> coop::Async<bool> override;

    // backend-specific
    template <class T, class... Args>
        requires(std::is_convertible_v<T*, ClientBackend*>)
    auto connect(T* backend, Args... args) -> coop::Async<bool> {
        co_return co_await connect_inner(backend, [backend, &args...] {
            return backend->connect(std::forward<Args>(args)...);
        }());
    }
};
} // namespace net::enc

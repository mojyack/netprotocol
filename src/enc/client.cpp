#include <coop/single-event.hpp>

#include "../macros/coop-unwrap.hpp"
#include "../util/concat.hpp"
#include "../util/random.hpp"
#include "client.hpp"
#include "crypto/c20p1305.hpp"
#include "crypto/x25519.hpp"

namespace net::enc {
auto ClientBackendEncAdaptor::connect_inner(ClientBackend* backend, coop::Async<bool> start) -> coop::Async<bool> {
    inner.reset(backend);

    // exchange key
    coop_unwrap(pair, crypto::x25519::generate());
    // setup temporary callbacks
    auto event         = coop::SingleEvent();
    inner->on_closed   = [&event] { event.notify(); };
    inner->on_received = [&pair, &event, this](const BytesRef payload) -> coop::Async<void> {
        auto opt = crypto::x25519::derive_secret(pair.priv, payload);
        if(opt) {
            secret = std::move(*opt);
        }
        event.notify();
        co_return;
    };

    coop_ensure(co_await start);
    auto job = inner->send(pair.pub);
    coop_ensure(co_await std::move(job));
    co_await event;
    coop_ensure(!secret.empty());
    cipher_context.reset(crypto::alloc_cipher_context());
    // key exhange complete

    // setup real callbacks
    inner->on_closed   = [this] { on_closed(); };
    inner->on_received = [this](const BytesRef payload) -> coop::Async<void> {
        co_ensure_v(payload.size() > crypto::c20p1305::iv_len);
        const auto iv   = payload.subspan(0, crypto::c20p1305::iv_len);
        const auto body = payload.subspan(crypto::c20p1305::iv_len);
        co_unwrap_v(dec, crypto::c20p1305::decrypt(cipher_context.get(), secret, iv, body));
        co_await on_received(dec);
    };
    co_return true;
}

auto ClientBackendEncAdaptor::send(BytesRef data) -> coop::Async<bool> {
    static auto engine = RandomEngine();

    const auto iv = engine.generate<crypto::c20p1305::iv_len>();
    coop_unwrap(enc, crypto::c20p1305::encrypt(cipher_context.get(), secret, iv, data));
    coop_ensure(co_await inner->send(concat(iv, enc)));
    co_return true;
}

auto ClientBackendEncAdaptor::finish() -> coop::Async<bool> {
    return inner->finish();
}
} // namespace net::enc

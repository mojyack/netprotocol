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
    coop_unwrap_mut(pair, crypto::x25519::generate());
    // setup temporary callbacks
    auto event         = coop::SingleEvent();
    inner->on_closed   = [&event] { event.notify(); };
    inner->on_received = [&pair, &event, this](const PrependableBuffer payload) -> coop::Async<void> {
        event.notify();
        co_unwrap_v(sec, crypto::x25519::derive_secret(pair.priv.body(), payload.body()));
        secret = copy(sec.body());
    };

    coop_ensure(co_await start);
    auto job = inner->send(std::move(pair.pub));
    coop_ensure(co_await std::move(job));
    co_await event;
    coop_ensure(secret.size() > 0);
    cipher_context.reset(crypto::alloc_cipher_context());
    // key exhange complete

    // setup real callbacks
    inner->on_closed   = [this] { on_closed(); };
    inner->on_received = [this](const PrependableBuffer payload) -> coop::Async<void> {
        const auto body = payload.body();
        co_ensure_v(body.size() > crypto::c20p1305::iv_len);
        const auto iv   = body.subspan(0, crypto::c20p1305::iv_len);
        const auto enc  = body.subspan(crypto::c20p1305::iv_len);
        auto       dec  = PrependableBuffer();
        const auto span = dec.enlarge(crypto::c20p1305::calc_decryption_buffer_size(enc.size()));
        co_ensure_v(crypto::c20p1305::decrypt(cipher_context.get(), secret, iv, enc, span));
        co_await on_received(std::move(dec));
    };
    co_return true;
}

auto ClientBackendEncAdaptor::send(PrependableBuffer data) -> coop::Async<bool> {
    static auto engine = RandomEngine();

    const auto body    = data.body();
    const auto enc_len = crypto::c20p1305::calc_encryption_buffer_size(body.size());
    auto       buf     = PrependableBuffer();
    buf.enlarge(crypto::c20p1305::iv_len + enc_len);
    const auto iv  = buf.body().subspan(0, crypto::c20p1305::iv_len);
    const auto enc = buf.body().subspan(crypto::c20p1305::iv_len);
    engine.random_fill_fixed_len<crypto::c20p1305::iv_len>(iv.data());
    coop_ensure(crypto::c20p1305::encrypt(cipher_context.get(), secret, iv, body, enc));
    coop_ensure(co_await inner->send(std::move(buf)));
    co_return true;
}

auto ClientBackendEncAdaptor::finish() -> coop::Async<bool> {
    return inner->finish();
}
} // namespace net::enc

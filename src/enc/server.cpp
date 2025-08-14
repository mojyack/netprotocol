#include "server.hpp"
#include "../common.hpp"
#include "../macros/coop-unwrap.hpp"
#include "../util/concat.hpp"
#include "../util/random.hpp"
#include "crypto/c20p1305.hpp"
#include "crypto/x25519.hpp"

namespace net::enc {
struct EncClientData : ClientData {
    ClientData* inner;
    BytesArray  secret;
};

namespace {
auto on_received_normal(ServerBackendEncAdaptor& self, const ClientData& inner_client, const BytesRef payload) -> coop::Async<void> {
    auto& client = *std::bit_cast<EncClientData*>(inner_client.data);
    coop_ensure(payload.size() > crypto::c20p1305::iv_len);
    const auto iv   = payload.subspan(0, crypto::c20p1305::iv_len);
    const auto enc  = payload.subspan(crypto::c20p1305::iv_len);
    auto       dec  = PrependableBuffer();
    const auto span = dec.enlarge(crypto::c20p1305::calc_decryption_buffer_size(enc.size()));
    coop_ensure(crypto::c20p1305::decrypt(self.cipher_context.get(), client.secret, iv, enc, span));
    co_await self.on_received(client, std::move(dec));
}

auto on_received_key_exchange(ServerBackendEncAdaptor& self, const ClientData& inner_client, const BytesRef payload) -> coop::Async<bool> {
    auto& client = *std::bit_cast<EncClientData*>(inner_client.data);
    // received peer pubkey
    coop_unwrap(pair, crypto::x25519::generate());
    coop_unwrap_mut(key, crypto::x25519::derive_secret(pair.priv.body(), payload));
    // we have secret now
    client.secret = copy(key.body());
    // send our pubkey
    coop_ensure(co_await self.inner->send(inner_client, std::move(pair.pub)));
    co_return true;
}
} // namespace

auto ServerBackendEncAdaptor::setup_inner(ServerBackend* backend) -> void {
    inner.reset(backend);

    cipher_context.reset(crypto::alloc_cipher_context());

    inner->alloc_client = [this](ClientData& client) -> coop::Async<void> {
        const auto ptr = new EncClientData();
        ptr->inner     = &client;
        client.data    = ptr;
        co_await alloc_client(*ptr);
    };
    inner->free_client = [this](void* vptr) -> coop::Async<void> {
        auto ptr = (EncClientData*)vptr;
        co_await free_client(ptr->data);
        delete ptr;
    };
    inner->on_received = [this](const ClientData& inner_client, PrependableBuffer payload) -> coop::Async<void> {
        auto& client = *std::bit_cast<EncClientData*>(inner_client.data);
        if(client.secret.empty()) {
            if(!co_await (on_received_key_exchange(*this, inner_client, payload.body()))) {
                WARN("key exchange failed");
                co_await inner->disconnect(inner_client);
            }
        } else {
            co_await on_received_normal(*this, inner_client, payload.body());
        }
    };
}

auto ServerBackendEncAdaptor::shutdown() -> coop::Async<bool> {
    return inner->shutdown();
}

auto ServerBackendEncAdaptor::disconnect(const ClientData& client) -> coop::Async<bool> {
    return inner->disconnect(client);
}

auto ServerBackendEncAdaptor::send(const ClientData& client, PrependableBuffer data) -> coop::Async<bool> {
    static auto engine = RandomEngine();

    auto&      tcpc    = *std::bit_cast<EncClientData*>(&client);
    const auto body    = data.body();
    const auto enc_len = crypto::c20p1305::calc_encryption_buffer_size(body.size());
    auto       buf     = PrependableBuffer();
    buf.enlarge(crypto::c20p1305::iv_len + enc_len);
    const auto iv  = buf.body().subspan(0, crypto::c20p1305::iv_len);
    const auto enc = buf.body().subspan(crypto::c20p1305::iv_len);
    engine.random_fill_fixed_len<crypto::c20p1305::iv_len>(iv.data());
    coop_ensure(crypto::c20p1305::encrypt(cipher_context.get(), tcpc.secret, iv, body, enc));
    coop_ensure(co_await inner->send(*tcpc.inner, std::move(buf)));
    co_return true;
}
} // namespace net::enc

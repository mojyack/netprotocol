#include "server.hpp"
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
    const auto body = payload.subspan(crypto::c20p1305::iv_len);
    coop_unwrap(dec, crypto::c20p1305::decrypt(self.cipher_context.get(), client.secret, iv, body));
    co_await self.on_received(client, dec);
}

auto on_received_key_exchange(ServerBackendEncAdaptor& self, const ClientData& inner_client, const BytesRef payload) -> coop::Async<bool> {
    auto& client = *std::bit_cast<EncClientData*>(inner_client.data);
    // received peer pubkey
    coop_unwrap(pair, crypto::x25519::generate());
    coop_unwrap_mut(key, crypto::x25519::derive_secret(pair.priv, payload));
    // we have secret now
    client.secret = std::move(key);
    // send our pubkey
    coop_ensure(co_await self.inner->send(inner_client, pair.pub));
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
    inner->on_received = [this](const ClientData& inner_client, BytesRef payload) -> coop::Async<void> {
        auto& client = *std::bit_cast<EncClientData*>(inner_client.data);
        if(client.secret.empty()) {
            if(!co_await (on_received_key_exchange(*this, inner_client, payload))) {
                WARN("key exchange failed");
                co_await inner->disconnect(inner_client);
            }
        } else {
            co_await on_received_normal(*this, inner_client, payload);
        }
    };
}

auto ServerBackendEncAdaptor::shutdown() -> coop::Async<bool> {
    return inner->shutdown();
}

auto ServerBackendEncAdaptor::disconnect(const ClientData& client) -> coop::Async<bool> {
    return inner->disconnect(client);
}

auto ServerBackendEncAdaptor::send(const ClientData& client, BytesRef data) -> coop::Async<bool> {
    static auto engine = RandomEngine();

    auto&      tcpc = *std::bit_cast<EncClientData*>(&client);
    const auto iv   = engine.generate<crypto::c20p1305::iv_len>();
    coop_unwrap(enc, crypto::c20p1305::encrypt(cipher_context.get(), tcpc.secret, iv, data));
    coop_ensure(co_await inner->send(*tcpc.inner, concat(iv, enc)));
    co_return true;
}
} // namespace net::enc

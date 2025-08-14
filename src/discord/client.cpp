#include <coop/task-handle.hpp>
#include <coop/timer.hpp>
#include <dpp/dpp.h>

#include <coop/generator.hpp>
#include <coop/promise.hpp>
#include <coop/select.hpp>
#include <coop/thread-event.hpp>
#include <coop/thread.hpp>

#include "../macros/coop-unwrap.hpp"
#include "../macros/logger.hpp"
#include "client.hpp"
#include "crypto/base64.hpp"

namespace net::discord {
namespace {
auto logger = Logger("discord");

constexpr auto connection_timeout = std::chrono::seconds(3);
constexpr auto keepalive_delay    = std::chrono::seconds(30);
constexpr auto keepalive_timeout  = std::chrono::seconds(10);
constexpr auto ratelimit_interval = std::chrono::milliseconds(2000);

struct MessageType {
    static constexpr auto Presence         = 'p';
    static constexpr auto Datagram         = 'd';
    static constexpr auto DatagramComplete = 'D';
    static constexpr auto Ping             = 'i';
    static constexpr auto Pong             = 'o';
};

struct Header {
    std::string_view name;
    std::string_view body;
    char             type;

    auto to_text() const -> std::string {
        return std::format("{}{}\n{}", type, name, body);
    }

    static auto from_text(std::string_view text) -> std::optional<Header> {
        // name
        LOG_DEBUG(logger, "parsing text {}", text);
        const auto nl = text.find('\n');
        if(nl == std::string::npos) {
            ensure(!text.empty());
            return Header{text.substr(1), {}, text[0]};
        }
        ensure(nl != std::string::npos);
        const auto before = text.substr(0, nl);
        const auto after  = text.substr(nl + 1);
        ensure(!before.empty() && !after.empty());
        return Header{before.substr(1), after, before[0]};
    }
};

auto send_rate_limit() -> coop::Async<void> {
    static auto wait_until = std::chrono::system_clock::time_point::min();
    if(const auto now = std::chrono::system_clock::now(); wait_until > now) {
        const auto delay = wait_until - now;
        wait_until += ratelimit_interval;
        LOG_DEBUG(logger, "delaying {} due to rate limit, next={}", delay, wait_until);
        co_await coop::sleep(delay);
    } else {
        wait_until = now + ratelimit_interval;
        LOG_DEBUG(logger, "time check passsed, next={}", wait_until);
    }
    co_return;
}

auto send_message(DiscordClient& self, const std::string_view text) -> coop::Async<bool> {
    co_await send_rate_limit();
    coop_ensure(self.cluster != nullptr && !self.cluster->terminating); // cluster maybe terminated while waiting

    auto result = false;
    auto done   = coop::ThreadEvent();
    self.cluster->message_create(dpp::message(self.channel_id, text), [&result, &done](dpp::confirmation_callback_t callback) {
        result = std::get_if<dpp::message>(&callback.value);
        if(!result) {
            WARN("{}", callback.get_error().human_readable);
        }
        done.notify();
    });
    co_await done;
    co_return result;
}

auto keepalive_task_main(DiscordClient& self) -> coop::Async<void> {
#define error_act goto fail
    LOG_DEBUG(logger, "{} keepalive timer reset", &self);
    co_await coop::sleep(keepalive_delay);
    LOG_DEBUG(logger, "{} sending ping", &self);
    ensure_a(co_await send_message(self, Header({self.name, {}, MessageType::Ping}).to_text()));
    co_await coop::sleep(keepalive_timeout);
fail:
    LOG_ERROR(logger, "connection disconnected");
    self.on_closed();
    co_return;
#undef error_act
}

auto handle_message(DiscordClient& self, const Header& packet, coop::SingleEvent& peer_appeard) -> coop::Async<bool> {
    LOG_DEBUG(logger, "{}: handling message type={}", self.name, packet.type);
    self.keepalive_task.cancel();
    self.injector->runner->push_task(keepalive_task_main(self), &self.keepalive_task);
    switch(packet.type) {
    case MessageType::Presence:
        if(!self.connected) {
            LOG_DEBUG(logger, "{}: peer appeard", self.name);
            peer_appeard.notify();
            self.connected = true;
            coop_ensure(co_await send_message(self, Header({self.name, {}, MessageType::Presence}).to_text()));
        }
        break;
    case MessageType::Datagram:
    case MessageType::DatagramComplete:
        LOG_DEBUG(logger, "{}: received datagram of length {}", self.name, packet.body.size());
        self.recv_buf += packet.body;
        if(packet.type == MessageType::DatagramComplete) {
            coop_unwrap(size, crypto::base64::calc_decode_buffer_size(self.recv_buf.size()));
            auto buf = PrependableBuffer();
            coop_unwrap(real_size, crypto::base64::decode(std::exchange(self.recv_buf, {}), buf.enlarge(size)));
            buf.resize(real_size);
            co_await self.on_received(std::move(buf));
        }
        break;
    case MessageType::Ping:
        if(self.connected) {
            coop_ensure(co_await send_message(self, Header({self.name, {}, MessageType::Pong}).to_text()));
        }
        break;
    case MessageType::Pong:
        break;
    default:
        coop_bail("unknown packet type {}", packet.type);
    }
    co_return true;
}
} // namespace

auto DiscordClient::send(PrependableBuffer data) -> coop::Async<bool> {
    const auto encoded = crypto::base64::encode(data.body());

    auto       text            = std::string_view(encoded);
    auto       header          = Header{name, {}, MessageType::Datagram}.to_text();
    const auto max_payload_len = 2000uz - header.size();
    while(!text.empty()) {
        const auto last     = text.size() < max_payload_len;
        const auto frag_len = last ? text.size() : max_payload_len;
        const auto frag     = std::string(text.substr(0, frag_len));
        header[0]           = last ? MessageType::DatagramComplete : MessageType::Datagram;
        coop_ensure(co_await send_message(*this, header + frag));
        text = text.substr(frag_len);
    }
    co_return true;
}

auto DiscordClient::finish() -> coop::Async<bool> {
    if(cluster == nullptr) {
        co_return true;
    }
    keepalive_task.cancel();
    co_await coop::run_blocking([this] { cluster->shutdown(); delete cluster; });
    cluster = nullptr;
    co_return true;
}

auto DiscordClient::connect(std::string name_, std::string peer_name_, uint64_t channel_id_, const std::string& token, coop::TaskInjector& injector_) -> coop::Async<bool> {
    this->name       = std::move(name_);
    this->peer_name  = std::move(peer_name_);
    this->channel_id = channel_id_;
    this->injector   = &injector_;
    this->cluster    = new dpp::cluster(token, dpp::i_default_intents | dpp::i_message_content);

    auto peer_appeard = coop::SingleEvent();
    cluster->on_message_create([this, &peer_appeard](dpp::message_create_t message) {
        ensure(!cluster->terminating);
        const auto& msg = message.msg;
        if(msg.channel_id != channel_id) {
            LOG_DEBUG(logger, "{}: channel id mismatched", name);
            return;
        }
        unwrap(packet, Header::from_text(message.msg.content));
        if(packet.name != peer_name) {
            LOG_DEBUG(logger, "{}: peer name mismatched {} != {}", name, packet.name, peer_name);
            return;
        }
        injector->inject_task(handle_message(*this, packet, peer_appeard));
    });
    {
        auto ready = coop::ThreadEvent();
        cluster->on_ready([&ready](dpp::ready_t /*ready*/) {
            ready.notify();
        });
        auto event_waiter = [&]() -> coop::Async<void> {
            co_await ready;
        };
        auto timeout = [&]() -> coop::Async<void> {
            co_await coop::sleep(connection_timeout);
        };
        cluster->start(dpp::st_return);
        coop_ensure(co_await coop::select(event_waiter(), timeout()) == 0);
    }
    LOG_INFO(logger, "{}: connected to discord", name);
    coop_ensure(co_await send_message(*this, Header({name, {}, MessageType::Presence}).to_text()));
    co_await peer_appeard;
    LOG_INFO(logger, "{}: connected to peer", name);

    co_return true;
}

DiscordClient::~DiscordClient() {
    if(cluster != nullptr) {
        injector->runner->await(finish());
    }
}
} // namespace net::discord

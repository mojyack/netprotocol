#pragma once
#include <coop/single-event.hpp>
#include <coop/task-injector.hpp>

#include "../backend.hpp"

namespace dpp {
class cluster;
}

namespace net::discord {
struct DiscordClient : ClientBackend {
    // private
    std::string         name;
    std::string         peer_name;
    uint64_t            channel_id;
    coop::TaskInjector* injector;
    dpp::cluster*       cluster = nullptr;
    std::string         recv_buf;
    coop::TaskHandle    keepalive_task;
    bool                connected = false;

    // overrides
    auto send(PrependableBuffer data) -> coop::Async<bool> override;
    auto finish() -> coop::Async<bool> override;

    auto connect(std::string name, std::string peer_name, uint64_t channel_id, const std::string& token, coop::TaskInjector& injector) -> coop::Async<bool>;

    ~DiscordClient();
};
} // namespace net::discord


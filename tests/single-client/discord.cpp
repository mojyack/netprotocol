#include "net/discord/client.hpp"
#include "util/argument-parser.hpp"

namespace {
using Client = net::discord::DiscordClient;

auto runner     = coop::Runner();
auto injector   = coop::TaskInjector(runner);
auto channel_id = uint64_t();
auto token      = (const char*)(nullptr);

auto prepare_client_1(Client&) -> void {
}

auto start_client_1(Client& client) -> coop::Async<bool> {
    co_return co_await client.connect("1", "2", channel_id, token, injector);
}

auto prepare_client_2(Client&) -> void {
}

auto start_client_2(Client& client) -> coop::Async<bool> {
    co_return co_await client.connect("2", "1", channel_id, token, injector);
}
} // namespace

#include "common.cpp"

namespace {
auto test_wrapper() -> coop::Async<void> {
    co_await test();
    injector.blocker.stop();
}
} // namespace

auto main(const int argc, const char* const* argv) -> int {
    {
        auto parser = args::Parser<uint64_t>();
        auto help   = false;
        parser.kwarg(&channel_id, {"-c", "--channel"}, "CHANNEL_ID", "channel id");
        parser.kwarg(&token, {"-t", "--token"}, "TOKEN", "discord bot token");
        parser.kwflag(&help, {"-h", "--help"}, "print this help message");
        if(!parser.parse(argc, argv) || help) {
            std::println("usage: discord {}", parser.get_help());
            return -1;
        }
    }
    runner.push_task(test_wrapper());
    runner.run();

    if(pass) {
        std::println("pass");
        return 0;
    } else {
        return -1;
    }
}

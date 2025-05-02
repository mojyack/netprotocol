#include <coop/generator.hpp>
#include <coop/promise.hpp>
#include <coop/task-handle.hpp>
#include <coop/timer.hpp>

#include "macros/coop-assert.hpp"
#include "util/charconv.hpp"
#include "util/cleaner.hpp"
#include "util/span.hpp"

namespace {
auto send_recv_test() -> coop::Async<bool> {
    auto c1 = Client();
    prepare_client_1(c1);
    auto c1_task   = coop::TaskHandle();
    auto cleaner   = Cleaner{[&c1_task] { c1_task.cancel(); }};
    c1.on_received = [&c1](net::BytesRef data) -> coop::Async<void> {
        std::println("received message from c1: {}", from_span(data));
        co_await c1.send(to_span(std::format("{}", data.size())));
        co_return;
    };
    (co_await coop::reveal_runner())->push_task(start_client_1(c1), &c1_task);

    auto message = std::string("hello");

    auto c2 = Client();
    prepare_client_2(c2);
    auto count     = 0uz;
    auto received  = coop::SingleEvent();
    c2.on_received = [&count, &received, &message](net::BytesRef data) -> coop::Async<void> {
        std::println("received message from c2: {}", from_span(data));
        if(const auto resp = from_chars<size_t>(from_span(data)); resp && *resp == message.size()) {
            count += 1;
            received.notify();
        }
        co_return;
    };

    coop_ensure(co_await start_client_2(c2));

    co_await c1_task.join();

    for(auto i = 0; i < 1; i += 1) {
        count = 0;
        for(auto i = 0; i < 3; i += 1) {
            coop_ensure(co_await c2.send(to_span(message)));
        }
        while(count != 3) {
            co_await received;
        }
        message += " hello";
    }

    co_return true;
}

auto close_test() -> coop::Async<bool> {
    auto c1 = Client();
    prepare_client_1(c1);
    auto c1_task = coop::TaskHandle();
    auto cleaner = Cleaner{[&c1_task] { c1_task.cancel(); }};
    (co_await coop::reveal_runner())->push_task(start_client_1(c1), &c1_task);

    auto closed  = coop::SingleEvent();
    auto c2      = Client();
    c2.on_closed = [&closed] { closed.notify(); };
    prepare_client_2(c2);
    coop_ensure(co_await start_client_2(c2));

    co_await c1_task.join();

    coop_ensure(co_await c1.finish());
    co_await closed;

    co_return true;
}

auto pass = false;

auto test() -> coop::Async<void> {
    coop_ensure(co_await send_recv_test());
    PRINT("send_recv_test pass")
    coop_ensure(co_await close_test());
    PRINT("close_test pass")
    pass = true;
}
} // namespace

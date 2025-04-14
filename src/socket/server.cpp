#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/task-handle.hpp>
#include <sys/socket.h>
#include <sys/types.h>

#include "../macros/coop-assert.hpp"
#include "../util/cleaner.hpp"
#include "macro.hpp"
#include "server.hpp"

namespace net::sock {
using SizeType = uint16_t;

auto SocketServerBackend::task_main() -> coop::Async<void> {
    auto& runner = *co_await coop::reveal_runner();

loop:
    coop_ensure(!(co_await coop::wait_for_file(sock.fd, true, false)).error);

    auto client_addr     = sockaddr_storage();
    auto client_addr_len = socklen_t(sizeof(client_addr));
    auto client          = sock::Socket(::accept(sock.fd, (sockaddr*)&client_addr, &client_addr_len));
    if(!client.is_valid()) {
        WARN("invalid client");
        goto loop;
    }

    auto iter  = client_data.emplace(client_data.end(), SocketClientData{});
    iter->sock = std::move(client);
    co_await alloc_client(*iter);
    runner.push_task(client_main(iter), &iter->task);

    goto loop;
}

auto SocketServerBackend::client_main(decltype(client_data)::iterator iter) -> coop::Async<void> {
    auto cleaner = Cleaner{[iter, this, &runner = *co_await coop::reveal_runner()] {
        runner.await(free_client(iter->data));
        iter->task.dissociate();
        client_data.erase(iter);
    }};

#define error_act co_return
loop:
    auto size = SizeType();
    sock_ensure(co_await iter->sock.read(&size, sizeof(size)));
    auto buffer = BytesArray(size);
    sock_ensure(co_await iter->sock.read(buffer.data(), size));
    co_await on_received(*iter, buffer);
    goto loop;
#undef error_act
}

auto SocketServerBackend::shutdown() -> coop::Async<bool> {
    while(!client_data.empty()) {
        client_data.front().task.cancel();
    }
    task.cancel();
    co_return true;
}

auto SocketServerBackend::disconnect(const ClientData& client) -> coop::Async<bool> {
    auto& tcpc = *std::bit_cast<SocketClientData*>(&client);
    tcpc.task.cancel();
    co_return true;
}

auto SocketServerBackend::send(const ClientData& client, const BytesRef data) -> coop::Async<bool> {
#define error_act co_return false
    auto&      tcpc = *std::bit_cast<SocketClientData*>(&client);
    const auto size = SizeType(data.size());
    coop_ensure(data.size() == size, "data too large {}", data.size());
    sock_ensure(co_await tcpc.sock.write(&size, sizeof(SizeType), MSG_MORE), false);
    sock_ensure(co_await tcpc.sock.write(data.data(), data.size()), false);
    co_return true;
#undef error_act
}

SocketServerBackend::~SocketServerBackend() {
    if(task.task != nullptr) {
        task.runner->await(shutdown());
    }
}
} // namespace net::sock

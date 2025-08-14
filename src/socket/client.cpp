#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/task-handle.hpp>
#include <coop/thread.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../macros/coop-assert.hpp"
#include "../util/cleaner.hpp"
#include "client.hpp"
#include "macro.hpp"

namespace net::sock {
using SizeType = uint16_t;

auto SocketClientBackend::task_main() -> coop::Async<void> {
    auto cleaner = Cleaner{[this] {
        sock.close();
        on_closed();
    }};

#define error_act co_return
loop:
    auto size = SizeType();
    sock_ensure(co_await sock.read(&size, sizeof(size)));
    auto buf = PrependableBuffer();
    sock_ensure(co_await sock.read(buf.enlarge(size).data(), size));
    co_await on_received(std::move(buf));
    goto loop;
#undef error_act
}

auto SocketClientBackend::send(PrependableBuffer data) -> coop::Async<bool> {
#define error_act co_return false
    const auto size = SizeType(data.size());
    coop_ensure(data.size() == size, "data too large {}", data.size());
    const auto header = data.enlarge_forward(sizeof(size));
    std::memcpy(header.data(), &size, sizeof(size));
    const auto body = data.body();
    sock_ensure(co_await sock.write(body.data(), body.size()), false);
    co_return true;
#undef error_act
}

auto SocketClientBackend::finish() -> coop::Async<bool> {
    task.cancel();
    co_return true;
}

SocketClientBackend::~SocketClientBackend() {
    if(task.task != nullptr) {
        task.runner->await(finish());
    }
}
} // namespace net::sock

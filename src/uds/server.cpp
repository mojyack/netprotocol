#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../macros/coop-assert.hpp"
#include "server.hpp"

namespace net::uds {
auto UDSServerBackend::start(const std::string_view path, const int backlog) -> coop::Async<bool> {
    coop_ensure(path.size() <= sizeof(sockaddr_un::sun_path) - 1);
    coop_ensure(sock::init_socket_system());
    sock = sock::Socket(socket(AF_UNIX, SOCK_STREAM, 0));
    coop_ensure(sock.is_valid());

    auto info       = sockaddr_un();
    info.sun_family = AF_UNIX;
    std::memcpy(info.sun_path, path.data(), path.size());
    coop_ensure(bind(sock.fd, (sockaddr*)&info, sizeof(info)) == 0, "{}({})", errno, strerror(errno));
    this->path = std::string(path);
    coop_ensure(listen(sock.fd, backlog) == 0);

    (co_await coop::reveal_runner())->push_task(task_main(), &task);

    co_return true;
}

UDSServerBackend::~UDSServerBackend() {
    if(!path.empty()) {
        unlink(path.data());
    }
}
} // namespace net::uds

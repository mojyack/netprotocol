#include <coop/io.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../macros/coop-assert.hpp"
#include "client.hpp"

namespace net::uds {
auto UDSClientBackend::connect(const std::string_view path) -> coop::Async<bool> {
    coop_ensure(path.size() <= sizeof(sockaddr_un::sun_path) - 1);
    coop_ensure(sock::init_socket_system());
    sock = sock::Socket(socket(AF_UNIX, SOCK_STREAM, 0));
    coop_ensure(sock.is_valid());

    auto info       = sockaddr_un();
    info.sun_family = AF_UNIX;
    std::memcpy(info.sun_path, path.data(), path.size());
    coop_ensure(sock.set_blocking(false));
    const auto status = ::connect(sock.fd, (sockaddr*)&info, sizeof(info));
    coop_ensure(status == 0 || (status == -1 && errno == EINPROGRESS));
    coop_ensure(!(co_await coop::wait_for_file(sock.fd, false, true)).error);
    coop_ensure(sock.get_sockopt(SO_ERROR) == 0);

    (co_await coop::reveal_runner())->push_task(task_main(), &task);

    co_return true;
}
} // namespace net::uds

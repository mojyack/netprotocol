#pragma once
#include <optional>
#include <utility>
#include <vector>

#include <coop/generator.hpp>
#include <unistd.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

namespace net::sock {
enum class ReadWriteResult {
    Success,
    Error,
    Abort,
};

struct Socket {
#if defined(_WIN32)
    using T                          = SOCKET;
    static constexpr auto invalid_fd = (T)INVALID_SOCKET;
#else
    using T                          = int;
    static constexpr auto invalid_fd = (T)-1;
#endif

    T fd;

    auto shutdown() -> void;
    auto close() -> void;
    auto is_valid() const -> bool;
    auto set_blocking(bool blocking) -> bool;
    auto set_sockopt(int name, int value) -> bool;
    auto set_sockopt(int level, int name, int value) -> bool;
    auto get_sockopt(int name) -> std::optional<int>;
    auto read(void* data, size_t size, int flag = 0) -> coop::Async<ReadWriteResult>;
    auto write(const void* data, size_t size, int flag = 0) -> coop::Async<ReadWriteResult>;
    auto read_all() -> std::optional<std::vector<std::byte>>;

    auto operator=(Socket&& o) -> Socket&;

    Socket() : fd(invalid_fd) {}
    Socket(const T data) : fd(data) {}
    Socket(Socket&& o) : fd(std::exchange(o.fd, invalid_fd)) {}
    ~Socket() {
        close();
    }
};

auto init_socket_system() -> bool;
auto build_ipv4_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint32_t;
} // namespace net::sock

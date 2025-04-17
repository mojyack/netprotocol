#include <coop/io.hpp>
#include <coop/promise.hpp>

#if !defined(_WIN32)
#include <fcntl.h>
#endif

#include "../macros/assert.hpp"
#include "socket.hpp"

namespace net::sock {
namespace {
auto rw(Socket& self, const bool read, void* data, size_t size, const int flag) -> coop::Async<ReadWriteResult> {
    constexpr auto error_value = ReadWriteResult::Error;

    auto ptr = std::bit_cast<std::byte*>(data);
    while(size > 0) {
        co_ensure_v(!(co_await coop::wait_for_file(self.fd, read, !read)).error);
        const auto result = read ? ::recv(self.fd, (char*)ptr, size, flag) : ::send(self.fd, (char*)ptr, size, flag);
        if(result == 0) {
            co_return ReadWriteResult::Abort;
        } else {
            co_ensure_v(result > 0);
        }
        size -= result;
        ptr += result;
    }
    co_return ReadWriteResult::Success;
}
} // namespace

auto Socket::shutdown() -> void {
#if defined(_WIN32)
    ::shutdown(fd, SD_BOTH);
#else
    ::shutdown(fd, SHUT_RDWR);
#endif
}

auto Socket::close() -> void {
    if(fd == invalid_fd) {
        return;
    }

#if defined(_WIN32)
    ::closesocket(fd);
#else
    ::close(fd);
#endif

    fd = invalid_fd;
}

auto Socket::is_valid() const -> bool {
    return fd != invalid_fd;
}

auto Socket::set_blocking(const bool blocking) -> bool {
#if defined(_WIN32)
    auto mode = u_long(blocking ? 0 : 1);
    ensure(ioctlsocket(fd, FIONBIO, &mode) == 0, WSAGetLastError());
#else
    auto flags = fcntl(fd, F_GETFL, 0);
    ensure(flags >= 0);
    if(blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    ensure(fcntl(fd, F_SETFL, flags) >= 0);
#endif
    return true;
}

auto Socket::set_sockopt(const int name, const int value) -> bool {
    return set_sockopt(SOL_SOCKET, name, value);
}

auto Socket::set_sockopt(const int level, const int name, const int value) -> bool {
    ensure(setsockopt(fd, level, name, &value, sizeof(value)) == 0);
    return true;
}

auto Socket::get_sockopt(const int name) -> std::optional<int> {
    auto error = int();
    auto len   = socklen_t(sizeof(error));
    ensure(getsockopt(fd, SOL_SOCKET, name, &error, &len) == 0);
    return error;
}

auto Socket::read(void* data, size_t size, const int flag) -> coop::Async<ReadWriteResult> {
    return rw(*this, true, data, size, flag);
}

auto Socket::write(const void* data, size_t size, const int flag) -> coop::Async<ReadWriteResult> {
    return rw(*this, false, const_cast<void*>(data), size, flag);
}

auto Socket::read_all() -> std::optional<std::vector<std::byte>> {
    constexpr auto read_size = 256;

    auto buffer = std::vector<std::byte>();
loop:
    ensure(set_blocking(buffer.empty()));
    const auto prev_size = buffer.size();
    buffer.resize(prev_size + read_size);
    const auto len = ::recv(fd, (char*)buffer.data() + prev_size, read_size, 0);

    const auto no_more_data =
#if defined(_WIN32)
        len == -1 && WSAGetLastError() == WSAEWOULDBLOCK;
#else
        len == -1 && errno == EAGAIN;
#endif
    buffer.resize(prev_size + std::max<int>(len, 0));
    if(no_more_data || len == 0) {
        return buffer;
    }
    ensure(len > 0);
    goto loop;
}

auto Socket::operator=(Socket&& o) -> Socket& {
    close();
    std::swap(fd, o.fd);
    return *this;
}

auto init_socket_system() -> bool {
#if defined(_WIN32)
    auto wsa_data = WSADATA();
    ensure(WSAStartup(WINSOCK_VERSION, &wsa_data) == 0);
#endif
    return true;
}

auto build_ipv4_addr(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) -> uint32_t {
    return a << 24 | b << 16 | c << 8 | d;
}
} // namespace net::sock

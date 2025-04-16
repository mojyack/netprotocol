#include "packet-parser.hpp"
#include "macros/coop-unwrap.hpp"

namespace net {
auto PacketParser::build_header(const PacketType pt, const size_t payload_size, const std::optional<PacketID> ref_id) -> std::optional<Header> {
    ensure(payload_size <= std::numeric_limits<decltype(Header::size)>::max(), "payload({}bytes) too large", payload_size);
    return Header{
        .type = pt,
        .id   = ref_id ? *ref_id : this->id += 1,
        .size = decltype(Header::size)(payload_size),
    };
}

auto PacketParser::send_header(coop::LockGuard&, const PacketType pt, const size_t payload_size, const std::optional<PacketID> ref_id) -> coop::Async<bool> {
    coop_unwrap(header, build_header(pt, payload_size, ref_id));
    coop_ensure(co_await send_data({std::bit_cast<std::byte*>(&header), sizeof(Header)}));
    co_return true;
}

auto PacketParser::parse_received(BytesRef data) -> std::optional<ParsedPacket> {
    if(!pending_header) {
        ensure(data.size() >= sizeof(Header));
        const auto& header = *std::bit_cast<Header*>(data.data());
        // PRINT("header received type={} size={}", header->type, header->size);
        if(data.size() != sizeof(Header) + header.size) {
            ensure(data.size() == sizeof(Header));
            pending_header = header;
            return std::nullopt;
        }
        return ParsedPacket{header, data.subspan(sizeof(Header))};
    } else {
        const auto header = *pending_header;
        pending_header.reset();
        ensure(data.size() == header.size, "{} != {}", data.size(), header.size);
        return ParsedPacket{header, data};
    }
}

auto PacketParser::send_packet(const PacketType pt, const void* const data, const size_t size, const std::optional<PacketID> ref_id) -> coop::Async<bool> {
    auto lock = co_await coop::LockGuard::lock(send_lock);
    coop_ensure(co_await send_header(lock, pt, size, ref_id));
    if(size > 0) {
        coop_ensure(co_await send_data({std::bit_cast<std::byte*>(data), size}));
    }
    co_return true;
}
} // namespace net

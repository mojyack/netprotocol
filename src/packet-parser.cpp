#include "packet-parser.hpp"
#include "macros/coop-assert.hpp"

namespace net {
auto PacketParser::send_packet(PacketType pt, PrependableBuffer packet, const std::optional<PacketID> ref_id) -> coop::Async<bool> {
    const auto size = packet.size();
    coop_ensure(size <= std::numeric_limits<decltype(Header::size)>::max(), "payload({}bytes) too large", size);
    packet.prepend_object(Header{
        .type = pt,
        .id   = ref_id ? * ref_id : this->id += 1,
        .size = decltype(Header::size)(size),
    });
    coop_ensure(co_await send_data(std::move(packet)));
    co_return true;
}

auto split_header(const BytesRef payload) -> std::optional<std::pair<Header, BytesRef>> {
    ensure(payload.size() >= sizeof(Header));
    return std::pair{
        *std::bit_cast<Header*>(payload.data()),
        payload.subspan(sizeof(Header)),
    };
}
} // namespace net

#pragma once
#include <functional>

#include <coop/promise.hpp>

#include "common.hpp"
#include "util/light-map.hpp"

namespace net {
struct Callbacks {
    using Callback = std::function<coop::Async<bool>(Header header, BytesRef payload)>;

    LightMap<PacketType, Callback> by_type;
    LightMap<PacketID, Callback>   by_id;

    auto invoke(Header header, BytesRef ref) -> coop::Async<bool>;
};
} // namespace net

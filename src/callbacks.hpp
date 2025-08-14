#pragma once
#include <functional>

#include <coop/promise.hpp>

#include "common.hpp"
#include "util/light-map.hpp"
#include "util/prependable-buffer.hpp"

namespace net {
struct Callbacks {
    using Callback = std::function<coop::Async<bool>(Header, PrependableBuffer)>;

    LightMap<PacketType, Callback> by_type;
    LightMap<PacketID, Callback>   by_id;

    auto invoke(Header header, PrependableBuffer buffer) -> coop::Async<bool>;
};
} // namespace net

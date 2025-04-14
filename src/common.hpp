#pragma once
#include <utility>

#include "serde/bin/format.hpp"

namespace net {
// common types
struct Header {
    uint8_t  type;
    uint8_t  id;
    uint16_t size;
};

using PacketType = decltype(Header::type);
using PacketID   = decltype(Header::id);

template <class T>
concept packet = (std::is_enum_v<decltype(T::pt)> && std::same_as<decltype(std::to_underlying(T::pt)), PacketType>) ||
                 (std::same_as<std::remove_const_t<decltype(T::pt)>, PacketType>);

using BinaryFormat = serde::BinaryFormat<uint16_t>;

using BytesArray = std::vector<std::byte>;
using BytesRef   = std::span<const std::byte>;
} // namespace net

#pragma once
#include <coop/generator.hpp>
#include <coop/lock-guard.hpp>
#include <coop/mutex.hpp>
#include <coop/promise.hpp>
#include <coop/single-event.hpp>

#include "callbacks.hpp"

#include "macros/micro.hpp"
#include "util/prependable-buffer.hpp"

namespace net {
struct PacketParser {
    Callbacks callbacks;
    PacketID  id = 0;

    std::function<coop::Async<bool>(PrependableBuffer data)> send_data; // set me

    // send serializable struct
    template <packet T>
    auto send_packet(T packet, std::optional<PacketID> ref_id = std::nullopt) -> coop::Async<bool>;

    // send raw data
    auto send_packet(PacketType pt, PrependableBuffer packet, std::optional<PacketID> ref_id = std::nullopt) -> coop::Async<bool>;

    // send packet then wait for the response(packet with the same id)
    template <class T>
    using ResponseType = std::conditional_t<serde::serde_struct<T>, std::optional<T>, bool>;
    template <packet Response, packet Request>
    auto receive_response(Request packet) -> coop::Async<ResponseType<Response>>;
};

template <packet T>
auto PacketParser::send_packet(const T packet, const std::optional<PacketID> ref_id) -> coop::Async<bool> {
    auto buf = PrependableBuffer();
    if constexpr(serde::serde_struct<T>) {
        buf.enlarge(0); // initialize storage
        unwrap(storage, packet.template dump<BinaryFormat>(std::move(buf.storage)), co_return false);
        buf.storage = std::move(storage);
    }
    ensure(co_await send_packet(T::pt, std::move(buf), ref_id), co_return false);
    co_return true;
}

template <packet Response, packet Request>
auto PacketParser::receive_response(Request packet) -> coop::Async<ResponseType<Response>> {
    const auto id = (this->id += 1);

    auto event          = coop::SingleEvent();
    auto response       = ResponseType<Response>();
    callbacks.by_id[id] = [&event, &response](const Header header, PrependableBuffer buffer) -> coop::Async<bool> {
        const auto ret = [&]() -> bool {
            ensure(header.type == Response::pt, return false);
            if constexpr(serde::serde_struct<Response>) {
                response = serde::load<BinaryFormat, Response>(buffer.body());
                ensure(response, return false);
            } else {
                response = true;
            }
            return true;
        }();
        event.notify();
        co_return ret;
    };

    constexpr auto error_value = ResponseType<Response>();
    ensure(co_await send_packet(std::move(packet), id), co_return error_value); // TODO: cancel callback on error
    co_await event;
    ensure(response, co_return error_value);
    co_return response;
}
} // namespace net

#include "macros/micro.hpp"

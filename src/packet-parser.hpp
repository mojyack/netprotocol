#pragma once
#include <coop/generator.hpp>
#include <coop/mutex.hpp>
#include <coop/promise.hpp>
#include <coop/single-event.hpp>

#include "callbacks.hpp"

#include "macros/micro.hpp"

namespace net {
struct PacketParser {
    Callbacks             callbacks;
    coop::Mutex           send_lock;
    std::optional<Header> pending_header;
    PacketID              id = 0;

    std::function<coop::Async<bool>(BytesRef data)> send_data; // set me

    auto build_header(PacketType pt, size_t payload_size = 0, std::optional<PacketID> ref_id = std::nullopt) -> std::optional<Header>;
    auto send_header(PacketType pt, size_t payload_size = 0, std::optional<PacketID> ref_id = std::nullopt) -> coop::Async<bool>;
    auto call_send_data(BytesRef data) -> coop::Async<bool>;

    // backend -> parser
    struct ParsedPacket {
        Header   header;
        BytesRef payload;
    };
    auto parse_received(BytesRef data) -> std::optional<ParsedPacket>;

    // user -> parser (-> backend)
    // send serializable struct
    template <packet T>
    auto send_packet(T packet, std::optional<PacketID> ref_id = std::nullopt) -> coop::Async<bool>;
    // send raw data
    auto send_packet(PacketType pt, const void* data, size_t size, std::optional<PacketID> ref_id = std::nullopt) -> coop::Async<bool>;
    // send packet then wait for the response(packet with the same id)
    template <class T>
    using ResponseType = std::conditional_t<serde::serde_struct<T>, std::optional<T>, bool>;
    template <packet Response, packet Request>
    auto receive_response(Request packet) -> coop::Async<ResponseType<Response>>;
};

template <packet T>
auto PacketParser::send_packet(const T packet, const std::optional<PacketID> ref_id) -> coop::Async<bool> {
    if constexpr(serde::serde_struct<T>) {
        unwrap(payload, packet.template dump<BinaryFormat>(BytesArray(sizeof(Header))), co_return false);
        unwrap(header, build_header(T::pt, payload.size() - sizeof(Header), ref_id), co_return false);
        *(std::bit_cast<Header*>(payload.data())) = header;
        ensure(co_await call_send_data(payload), co_return false);
    } else {
        ensure(co_await send_header(T::pt, 0, ref_id), co_return false);
    }
    co_return true;
}

template <packet Response, packet Request>
auto PacketParser::receive_response(Request packet) -> coop::Async<ResponseType<Response>> {
    const auto id = (this->id += 1);

    auto event          = coop::SingleEvent();
    auto response       = ResponseType<Response>();
    callbacks.by_id[id] = [&event, &response](const Header header, const BytesRef payload) -> coop::Async<bool> {
        const auto ret = [&]() -> bool {
            ensure(header.type == Response::pt, return false);
            if constexpr(serde::serde_struct<Response>) {
                response = serde::load<BinaryFormat, Response>(payload);
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

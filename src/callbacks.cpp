#include <coop/generator.hpp>

#include "callbacks.hpp"
#include "macros/coop-assert.hpp"

namespace net {
auto Callbacks::invoke(const Header header, const BytesRef payload) -> coop::Async<bool> {
    // PRINT("type={} id={}", header.type, header.id);
    if(const auto i = by_type.find(header.type); i != by_type.end()) {
        // PRINT("type matched");
        coop_ensure(co_await i->second(header, payload));
    } else if(const auto i = by_id.find(header.id); i != by_id.end()) {
        // PRINT("id matched");
        const auto cb = std::move(i->second);
        by_id.erase(i);
        coop_ensure(co_await cb(header, payload));
    } else {
        coop_bail("no callback registered for id={} type={}", header.id, header.type);
    }
    co_return true;
}
} // namespace net

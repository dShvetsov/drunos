#pragma once

#include <oxm/field_set.hh>
#include "policies.hh"

namespace runos {
namespace retic {

class Backend {
public:
    virtual void install(
        oxm::field_set match,
        std::vector<oxm::field_set> action,
        uint16_t piority,
        FlowSettings flow_settings
    ) = 0;
    virtual void installBarrier(
        oxm::field_set match,
        uint16_t priority
    ) = 0;
    virtual void packetOuts(
        uint8_t* data,
        size_t data_len,
        std::vector<oxm::field_set> actions,
        uint64_t dpid
    ) = 0;
    virtual ~Backend() = default;
};

} // namespace retic
} // namespace runos

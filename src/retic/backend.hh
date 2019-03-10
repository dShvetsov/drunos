#pragma once

#include <oxm/field_set.hh>

namespace runos {
namespace retic {

class Backend {
public:
    virtual void install(
        oxm::field_set match,
        std::vector<oxm::field_set> action,
        uint16_t piority
    ) = 0;
    virtual void installBarrier(
        oxm::field_set match,
        uint16_t priority
    ) = 0;
    virtual ~Backend() = default;
};

} // namespace retic
} // namespace runos

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
};

} // namespace retic
} // namespace runos

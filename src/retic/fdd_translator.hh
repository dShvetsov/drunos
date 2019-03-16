#pragma once

#include <optional>
#include <boost/variant/static_visitor.hpp>

#include "fdd.hh"
#include "backend.hh"

namespace runos {
namespace retic {
namespace fdd {

class Translator : boost::static_visitor<> {
public:
    Translator(Backend& backend) : m_backend(backend) { }

    void operator()(const node& n);
    void operator()(const leaf& l);
private:
    Backend& m_backend;
    oxm::field_set match;
    uint16_t prio_up = 65535u;
    uint16_t prio_middle = 0;
    std::optional<oxm::mask<>> previous_mask = std::nullopt;
    uint16_t prio_down = 1;
};

} // namespace fdd
} // namespace retic
} // namespace runos

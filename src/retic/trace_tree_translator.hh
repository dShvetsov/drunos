#pragma once

#include <boost/variant/static_visitor.hpp>

#include "trace_tree.hh"
#include "backend.hh"


namespace runos {
namespace retic {
namespace trace_tree {

class Translator: boost::static_visitor<> {
public:
    Translator(Backend& backend, oxm::field_set pre_match, uint16_t prio_up, uint16_t prio_down)
        : m_backend(backend)
        , match(pre_match)
        , prio_up(prio_up)
        , prio_down(prio_down)
    { }

    Translator(Backend& backend)
        : m_backend(backend)
    { }

    void operator()(const unexplored& u);
    void operator()(const leaf_node& ln);
    void operator()(const test_node& tn);
    void operator()(const load_node& ln);

private:
    Backend& m_backend;
    oxm::field_set match;
    uint16_t prio_up = 65535;
    uint16_t prio_down = 1;
};

} // namespace trace_tree
} // namespace retic
} // namespace runos

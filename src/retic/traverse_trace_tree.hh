#pragma once

#include <tuple>

#include <boost/variant/static_visitor.hpp>

#include "trace_tree.hh"
#include "oxm/field_set.hh"

namespace runos {
namespace retic {
namespace trace_tree {

namespace traverser {
using ret_type = std::pair<
    std::shared_ptr<fdd::diagram_holder>,
    oxm::field_set
>;
} // namespace traverser

// Traverse the tree and return pointer to diagram holder and match collected over the trace
// If there is no node then return nullptr, and match may be any
class Traverser: public boost::static_visitor<traverser::ret_type>
{
public:
    using ret_type = std::pair<
        std::shared_ptr<fdd::diagram_holder>,
        oxm::field_set
    >;

    Traverser(const Packet& pkt): m_pkt(pkt) { }
    traverser::ret_type operator()(unexplored& n);
    traverser::ret_type operator()(leaf_node& n);
    traverser::ret_type operator()(load_node& ln);
    traverser::ret_type operator()(test_node& tn);

private:
    const Packet& m_pkt;
    oxm::field_set m_match;
};

}
}
}

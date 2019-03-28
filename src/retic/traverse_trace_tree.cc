#include "traverse_trace_tree.hh"

namespace runos {
namespace retic {
namespace trace_tree {

using namespace traverser;

ret_type Traverser::operator()(unexplored& n) {
    return {nullptr, m_match};
}

ret_type Traverser::operator()(load_node& load)
{
    auto it = load.cases.find( m_pkt.load(load.mask).value_bits() );
    if (it != load.cases.end()) {
        oxm::type t = load.mask.type();
        m_match.modify( (t == it->first) & load.mask  );
        return boost::apply_visitor(*this, it->second);
    } else {
        return {nullptr, m_match};
    }
}

ret_type Traverser::operator()(test_node& test) {
        if (m_pkt.test(test.need)) {
            m_match.modify(test.need);
            return boost::apply_visitor(*this, test.positive);
        } else {
            return boost::apply_visitor(*this, test.negative);
        }
}

ret_type Traverser::operator()(leaf_node& ln) {
    return {ln.kat_diagram, m_match};
}

} // namespace trace_tree
} // retic
} // runos


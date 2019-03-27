#include "traverse_trace_tree.hh"

namespace runos {
namespace retic {
namespace trace_tree {

std::shared_ptr<fdd::diagram_holder> Traverser::operator()(unexplored& n) {
    return nullptr;
}

std::shared_ptr<fdd::diagram_holder> Traverser::operator()(load_node& load)
{
    auto it = load.cases.find( m_pkt.load(load.mask).value_bits() );
    if (it != load.cases.end()) {
        return boost::apply_visitor(*this, it->second);
    } else {
        return nullptr;
    }
}

std::shared_ptr<fdd::diagram_holder> Traverser::operator()(test_node& test) {
        if (m_pkt.test(test.need)) {
            return boost::apply_visitor(*this, test.positive);
        } else {
            return boost::apply_visitor(*this, test.negative);
        }
}

std::shared_ptr<fdd::diagram_holder> Traverser::operator()(leaf_node& ln) {
    return ln.kat_diagram;
}

} // namespace trace_tree
} // retic
} // runos


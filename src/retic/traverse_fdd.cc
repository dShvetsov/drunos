#include "traverse_fdd.hh"

#include "traverse_trace_tree.hh"

namespace runos {
namespace retic{
namespace fdd {

leaf& Traverser::operator()(leaf& l) {
    return l;
}

leaf& Traverser::operator()(node& n) {
    return m_pkt.test(n.field) ? boost::apply_visitor(*this, n.positive)
                               : boost::apply_visitor(*this, n.negative);
}

} // fdd
} // retic
} // runos

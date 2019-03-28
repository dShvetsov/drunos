#include "traverse_fdd.hh"

#include <algorithm>

#include "traverse_trace_tree.hh"
#include "trace_tree.hh"
#include "tracer.hh"
#include "leaf_applier.hh"

namespace runos {
namespace retic{
namespace fdd {

leaf& Traverser::operator()(leaf& l) {

    if (std::any_of(
            l.sets.begin(), l.sets.end(),
            [](auto& x){ return x.body.has_value(); }
    )) {
        // has trace_tree in leaf
        trace_tree::Traverser traverser{m_pkt};
        auto [next_fdd, maple_match] = boost::apply_visitor(traverser, l.maple_tree);
        if (next_fdd == nullptr) {
            // has no value for this packet
            // should create it
            // TODO: use pre_match for mergeTraces and so on
            auto traces = retic::getTraces(l, m_pkt);
            auto merged_trace = tracer::mergeTrace(traces);
            trace_tree::Augmention augmenter( &(l.maple_tree) );
            for (auto& n: merged_trace.values()) {
                boost::apply_visitor(augmenter, n);
            }
            next_fdd = augmenter.finish(merged_trace.result());
        }

        return boost::apply_visitor(*this, next_fdd->value);

    } else {
        // no trace_trees in leaf
        return l;
    }
}

leaf& Traverser::operator()(node& n) {
    return m_pkt.test(n.field) ? boost::apply_visitor(*this, n.positive)
                               : boost::apply_visitor(*this, n.negative);
}

} // fdd
} // retic
} // runos

#include "traverse_fdd.hh"

#include <algorithm>

#include "traverse_trace_tree.hh"
#include "trace_tree.hh"
#include "tracer.hh"
#include "leaf_applier.hh"

namespace runos {
namespace retic{
namespace fdd {

namespace {
// Just find the leaf of fdd by packet
// No recursive traverse of trace trees
class PlainTraverser: public boost::static_visitor<const leaf&> {
public:
    PlainTraverser(const Packet& pkt)
        : m_pkt(pkt)
    { }
    const leaf& operator()(const leaf& l) const {
        return l;
    }
    const leaf& operator()(const node& n) const {
        return m_pkt.test(n.field) ? boost::apply_visitor(*this, n.positive)
                                   : boost::apply_visitor(*this, n.negative);
    }
private:
    const Packet& m_pkt;
};

// leaf of Packet hard_timeout == duration::zero()
bool leaf_is_temporary(const Packet& pkt, const diagram& d) {
    auto& l = boost::apply_visitor(PlainTraverser{pkt}, d);
    return l.flow_settings.hard_timeout == duration::zero();
}

}

leaf& Traverser::operator()(leaf& l) {

    if (std::any_of(
            l.sets.begin(), l.sets.end(),
            [](auto& x){ return x.body.has_value(); }
    )) {
        // has trace_tree in leaf
        trace_tree::Traverser traverser{m_pkt};
        auto [next_fdd, maple_match] = boost::apply_visitor(traverser, l.maple_tree);
        if (next_fdd == nullptr or leaf_is_temporary(m_pkt, next_fdd->value)) {
            // has no value for this packet
            // should create it
            auto traces = retic::getTraces(l, m_pkt);
            auto merged_trace = tracer::mergeTrace(traces, m_match);
            trace_tree::Augmention augmenter(
                &(l.maple_tree), m_backend, m_match, l.prio_down, l.prio_up
            );
            for (auto& n: merged_trace.values()) {
                boost::apply_visitor(augmenter, n);
            }
            next_fdd = augmenter.finish(merged_trace.result());
            m_match = augmenter.match();
        }

        for (auto& f: maple_match) {
            m_match.modify(f);
        }

        return boost::apply_visitor(*this, next_fdd->value);

    } else {
        // no trace_trees in leaf
        return l;
    }
}

leaf& Traverser::operator()(node& n) {
    if (m_pkt.test(n.field)) {
        m_match.modify(n.field);
        return boost::apply_visitor(*this, n.positive);
    } else {
       return boost::apply_visitor(*this, n.negative);
    }
}

} // fdd
} // retic
} // runos

#include "tracer.hh"

#include <iostream>

#include "maple/TraceablePacketImpl.hh"
#include "oxm/field_set.hh"

namespace runos {
namespace retic {
namespace tracer {


// TODO: merge with src/Maple.cc ModTracingPacket
struct ModTrackingPacket final : public PacketProxy {
    oxm::field_set m_mods;
public:
    explicit ModTrackingPacket(Packet& pkt)
        : PacketProxy(pkt)
    { }

    void modify(const oxm::field<> patch) override
    {
        pkt.modify(patch);
        m_mods.modify(patch);
    }

    const oxm::field_set& mods() const
    { return m_mods; }
};

void Trace::load(oxm::field<> unexplored) {
    auto ln = load_node{unexplored};
    m_trace_impl.push_back(ln);
}

void Trace::test(oxm::field<> field, bool result) {
    auto tn = test_node{field, result};
    m_trace_impl.push_back(tn);
}


Trace Tracer::trace(Packet& pkt) const {
    Trace ret;
    maple::TraceablePacketImpl traceable_pkt(pkt, ret);
    ModTrackingPacket mod_tracking(traceable_pkt);
    policy p = m_packet_handler(mod_tracking);
    for (auto& mod: mod_tracking.mods()) {
        p = modify(mod) >> p;
    }
    ret.setResult(p);
    return ret;
}


Trace mergeTrace(const std::vector<Trace>& traces, oxm::field_set cache) {
    Trace ret;

    struct Handler: public boost::static_visitor<> {
        oxm::field_set& cache;
        Trace& trace;
        Handler(oxm::field_set& cache, Trace& trace)
            : cache(cache), trace(trace)
        { }
        void operator()(const load_node& ln) {
            oxm::mask<> mask(ln.field);
            oxm::field<> explored = cache.load(mask);
            oxm::field<> unexplored = ln.field & ~oxm::mask<>(explored);

            if (not unexplored.wildcard()) {
                // if we hasn't this value yet
                trace.load(unexplored);
                cache.modify(unexplored);
            }
        }

        void operator()(const test_node& tn) {
            oxm::mask<> mask(tn.field);
            oxm::field<> explored = cache.load(mask);
            oxm::field<> unexplored = tn.field & ~oxm::mask<>(explored);
            if (not unexplored.wildcard()) {
                trace.test(unexplored, tn.result);
                if (tn.result) {
                    // cached value only if its positive result
                    cache.modify(unexplored);
                }
            }
        }
    };

    Handler handler(cache, ret);
    for (auto& trace: traces) {
        for (auto& x: trace) {
            boost::apply_visitor(handler, x); 
        }
    }

    auto it = traces.begin();
    if (it == traces.end()) {
        throw std::logic_error("There is not possibly way to merge zero traces");
    }

    policy result_policy = it->result();
    it++;
    while (it != traces.end()) {
        result_policy = result_policy + it->result();
        it++;
    }
    ret.setResult(result_policy);
    return ret;
}

} // namespace tracer
} // namespace retic 
} // namespace runos

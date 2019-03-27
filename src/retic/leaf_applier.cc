#include "leaf_applier.hh"

namespace runos {
namespace retic {

std::vector<tracer::Trace> getTraces(const fdd::leaf& l, const Packet& orig_pkt) {
    std::vector<tracer::Trace> ret;
    ret.reserve(l.sets.size());
    for (auto& unit: l.sets) {
        policy wrapped_handler = handler([&unit](Packet& pkt) mutable {
            for (const oxm::field<>& f: unit.pred_actions) {
                pkt.modify(f);
            }
            return unit.body.has_value() ? unit.body.value().function(pkt) : id();
        });
        auto pkt = orig_pkt.clone();
        tracer::Tracer tracer(wrapped_handler);
        tracer::Trace tr = tracer.trace(*pkt);
        ret.push_back(tr);
    }
    return ret;
}

} // namespace retic
} // namespace runos

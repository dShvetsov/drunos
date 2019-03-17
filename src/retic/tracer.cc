#include "tracer.hh"

#include "maple/TraceablePacketImpl.hh"

namespace runos {
namespace retic {
namespace tracer {

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
    auto f = boost::get<PacketFunction>(m_policy);
    ret.setResult(f.function(traceable_pkt));
    return ret;
}

} // namespace tracer
} // namespace retic 
} // namespace runos

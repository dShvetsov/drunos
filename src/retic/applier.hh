#pragma once

#include "policies.hh"
#include "api/Packet.hh"

namespace runos {
namespace retic {

class Meta {
public:
    bool isStopped() const {
        return m_is_stopped;
    }

    void stop() {
        m_is_stopped = true;
    }
private:
    bool m_is_stopped = false;
};

class Applier : public boost::static_visitor<> {
    public:
    using PacketPtr = std::shared_ptr<Packet>;
    using PacketsWithMeta = std::vector<std::pair<PacketPtr, Meta>>;
    Applier(Packet& pkt)
        : m_pkts{{pkt.clone(), Meta{}}}
    { }

    Applier(const PacketsWithMeta& pkts)
        : m_pkts(pkts)
    { }

    void operator()(const Filter& fil) {
        for (auto& [pkt, meta] : m_pkts) {
            if (!pkt->test(fil.field)) {
                meta.stop();
            }
        }
    }

    void operator()(const Stop& stop) {
        for (auto& [pkt, meta] : m_pkts) {
            meta.stop();
        }
    }

    void operator()(const Modify& mod) {
        for (auto& [pkt, res] : m_pkts) {
            pkt->modify(mod.field);
        }
    }

    void operator()(const Sequential& seq) {
        boost::apply_visitor(*this, seq.one);
        if (not m_pkts.at(0).second.isStopped()) {
            boost::apply_visitor(*this, seq.two);
        }
    }

    void operator()(const Id& id) {
        // do nothing with packet
    }

    void operator()(const Parallel& par) {
        PacketsWithMeta cloned;
        cloned.reserve(m_pkts.size());
        for (auto& [pkt, meta]: m_pkts) {
            cloned.push_back({pkt->clone(), meta});
        }
        boost::apply_visitor(*this, par.one);
        std::swap(cloned, m_pkts);
        boost::apply_visitor(*this, par.two);

        std::swap(cloned, m_pkts);
        m_pkts.reserve(m_pkts.size() + cloned.size());
        m_pkts.insert(m_pkts.end(), cloned.begin(), cloned.end());
    }

    void operator()(const PacketFunction& func) {
        for (auto& [pkt, meta] : m_pkts) {
            if (not meta.isStopped()) {
                policy p = func.function(*pkt);
                boost::apply_visitor(*this, p);
            }
        }
    }

    const PacketsWithMeta& results() const
    { return m_pkts; }

private:

    PacketsWithMeta m_pkts;
};

} // namespace retic
} // namespace runos

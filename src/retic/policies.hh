#pragma once

#include <functional>
#include <list>
#include <set>
#include <tuple>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>
#include <boost/variant/static_visitor.hpp>
#include "api/Packet.hh"
#include "oxm/openflow_basic.hh"

namespace runos {

class Filter {
public:
    oxm::field<> field;
};

class Stop { };

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

struct Modify {
    oxm::field<> field;
};

struct Sequential;
struct Parallel;
struct PacketFunction;

using policy =
    boost::variant<
        Filter,
        Modify,
        Stop,
        boost::recursive_wrapper<PacketFunction>,
        boost::recursive_wrapper<Sequential>,
        boost::recursive_wrapper<Parallel>
    >;

struct PacketFunction {
    std::function<policy(Packet& pkt)> function;
};

struct Sequential {
    policy one;
    policy two;
};

struct Parallel {
    policy one;
    policy two;
};

policy modify(oxm::field<> field) {
    return Modify{field};
}

policy fwd(uint32_t port) {
    static oxm::out_port out_port;
    return modify(out_port << port);
};

policy stop() {
    return Stop();
}

policy filter(oxm::field<> f)
{
    return Filter{f};
}

policy handler(std::function<policy(Packet&)> function)
{
    return PacketFunction{function};
}

policy operator>>(policy lhs, policy rhs)
{
    return Sequential{lhs, rhs};
}

policy operator+(policy lhs, policy rhs)
{
    return Parallel{lhs, rhs};
}


template <class P> // enable if P is baseof Packet
class Applier : public boost::static_visitor<> {
    public:

    using PacketType = P;
    using PacketsWithMeta = std::vector<std::pair<PacketType, Meta>>;

    Applier(PacketType pkt)
        : m_pkts{{std::forward<PacketType>(pkt), Meta{}}}
    { }

    Applier(const PacketsWithMeta& pkts)
        : m_pkts(pkts)
    { }

    void operator()(const Filter& fil) {
        for (auto& [pkt, meta] : m_pkts) {
            if (!pkt.test(fil.field)) {
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
            pkt.modify(mod.field);
        }
    }

    void operator()(const Sequential& seq) {
        boost::apply_visitor(*this, seq.one);
        if (not m_pkts.at(0).second.isStopped()) {
            boost::apply_visitor(*this, seq.two);
        }
    }

    void operator()(const Parallel& par) {
        auto save_pkts = m_pkts;
        boost::apply_visitor(*this, par.one);
        std::swap(save_pkts, m_pkts);
        boost::apply_visitor(*this, par.two);

        std::swap(save_pkts, m_pkts);
        m_pkts.reserve(m_pkts.size() + save_pkts.size());
        m_pkts.insert(m_pkts.end(), save_pkts.begin(), save_pkts.end());
    }

    void operator()(const PacketFunction& func) {
        for (auto& [pkt, meta] : m_pkts) {
            if (not meta.isStopped()) {
                policy p = func.function(pkt);
                boost::apply_visitor(*this, p);
            }
        }
    }

    const PacketsWithMeta& results() const
    { return m_pkts; }

private:

    PacketsWithMeta m_pkts;
};

}// namespace runos

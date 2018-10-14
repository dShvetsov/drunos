#pragma once

#include <set>
#include <list>
#include <tuple>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>
#include <boost/variant/static_visitor.hpp>
#include "api/Packet.hh"

namespace runos {

class Result {
public:
    bool stopped = false;
    uint32_t port;
};

class Filter {
public:
    oxm::field<> field;
};

class Stop { };


struct Forward {
    uint32_t port;
};

struct Modify {
    oxm::field<> field;
};

struct Sequential;
struct Parallel;

using policy =
    boost::variant<
        Filter,
        Forward,
        Modify,
        Stop,
        boost::recursive_wrapper<Sequential>,
        boost::recursive_wrapper<Parallel>
    >;

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
    return Forward{port};
};

policy stop() {
    return Stop();
}

policy filter(oxm::field<> f)
{
    return Filter{f};
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
    Applier(P pkt)
        : m_pkts{{std::forward<P>(pkt), Result{}}}
    { }

    void operator()(const Filter& fil) {
        for (auto& [pkt, res] : m_pkts) {
            res.stopped = !pkt.test(fil.field);
        }
    }

    void operator()(const Stop& stop) {
        for (auto& [pkt, res] : m_pkts) {
            res.stopped = true;
        }
    }

    void operator()(const Forward& fwd) {
        for (auto& [pkt, res] : m_pkts) {
            res.port = fwd.port;
        }
    }

    void operator()(const Modify& mod) {
        for (auto& [pkt, res] : m_pkts) {
            pkt.modify(mod.field);
        }
    }

    void operator()(const Sequential& seq) {
        boost::apply_visitor(*this, seq.one);
        if (not m_pkts.at(0).second.stopped) {
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

    const std::vector<std::pair<P, Result>>& results() const
    { return m_pkts; }

private:
    std::vector<std::pair<P, Result>> m_pkts;
};

}// namespace runos

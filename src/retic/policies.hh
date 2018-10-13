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

Filter filter(oxm::field<> f)
{
    return Filter{f};
}

class Stop { };

Stop stop() {
    return Stop();
}

struct Forward {
    uint32_t port;
};

Forward fwd(uint32_t port) {
    return Forward{port};
};

struct Modify {
    oxm::field<> field;
};

Modify modify(oxm::field<> field) {
    return Modify{field};
}


struct Sequential;

using policy =
    boost::variant<
        Filter,
        Forward,
        Modify,
        Stop,
        boost::recursive_wrapper<Sequential>

    >;

struct Sequential {
    policy one;
    policy two;
};


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

    const std::vector<std::pair<P, Result>>& results() const
    { return m_pkts; }

private:
    std::vector<std::pair<P, Result>> m_pkts;
};

}// namespace runos

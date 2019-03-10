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
namespace retic {

class Filter {
public:
    oxm::field<> field;
};

class Stop { };

struct Modify {
    oxm::field<> field;
};

struct Sequential;
struct Parallel;
struct PacketFunction;

using policy =
    boost::variant<
        Stop,
        Filter,
        Modify,
        boost::recursive_wrapper<PacketFunction>,
        boost::recursive_wrapper<Sequential>,
        boost::recursive_wrapper<Parallel>
    >;

struct PacketFunction {
    uint64_t id;
    std::function<policy(Packet& pkt)> function;
    friend bool operator==(const PacketFunction& lhs, const PacketFunction& rhs) {
        return lhs.id == rhs.id;
    }
};

struct Sequential {
    policy one;
    policy two;
};

struct Parallel {
    policy one;
    policy two;
};

inline
policy modify(oxm::field<> field) {
    return Modify{field};
}

inline
policy fwd(uint32_t port) {
    static constexpr auto out_port = oxm::out_port();
    return modify(out_port << port);
};

inline
policy stop() {
    return Stop();
}

inline
policy filter(oxm::field<> f)
{
    return Filter{f};
}

inline
policy handler(std::function<policy(Packet&)> function)
{
    static uint64_t id_gen = 0;
    return PacketFunction{id_gen++, function};
}

inline
policy operator>>(policy lhs, policy rhs)
{
    return Sequential{lhs, rhs};
}

inline
policy operator+(policy lhs, policy rhs)
{
    return Parallel{lhs, rhs};
}

// This is only for purpose that seq operator must have higher priorty that parallel operator
inline
policy operator|(policy lhs, policy rhs)
{
    return Parallel{lhs, rhs};
}

} // namespace retic
} // namespace runos

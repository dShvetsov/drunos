#pragma once

#include <functional>
#include <list>
#include <set>
#include <tuple>
#include <chrono>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>
#include <boost/variant/static_visitor.hpp>
#include "api/Packet.hh"
#include "oxm/openflow_basic.hh"

namespace runos {
namespace retic {

typedef std::chrono::duration<uint32_t> duration;

class Filter {
public:
    oxm::field<> field;
};

class Stop { };

class Id { };

struct Modify {
    oxm::field<> field;
};

struct FlowSettings {
    duration idle_timeout = duration::max();
    duration hard_timeout = duration::max();
    inline friend FlowSettings operator&(const FlowSettings& lhs, const FlowSettings& rhs) {
        FlowSettings ret;
        ret.idle_timeout = std::min(lhs.idle_timeout, rhs.idle_timeout);
        ret.hard_timeout = std::min(lhs.hard_timeout, rhs.hard_timeout);

        // hard timeout cann't be less than idle timeout
        ret.idle_timeout = std::min(ret.idle_timeout, ret.hard_timeout);
        return ret;
    }
};

struct Sequential;
struct Parallel;
struct PacketFunction;
struct Negation;

using policy =
    boost::variant<
        Stop,
        Id,
        Filter,
        Modify,
        FlowSettings,
        boost::recursive_wrapper<Negation>,
        boost::recursive_wrapper<PacketFunction>,
        boost::recursive_wrapper<Sequential>,
        boost::recursive_wrapper<Parallel>
    >;

struct Negation {
    policy pol;
};

struct PacketFunction {
    uint64_t id;
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
policy id() {
    return Id();
}

inline
policy filter(oxm::field<> f)
{
    return Filter{f};
}

inline
policy filter_not(oxm::field<>f ) {
    return Negation{Filter{f}};
}

inline
policy handler(std::function<policy(Packet&)> function)
{
    static uint64_t id_gen = 0;
    return PacketFunction{id_gen++, function};
}

inline
policy idle_timeout(duration time) {
    return FlowSettings{.idle_timeout = time};
}

inline
policy hard_timeout(duration time) {
    return FlowSettings{time, time};
}

inline
policy operator>>(const policy& lhs, const policy& rhs)
{
    return Sequential{lhs, rhs};
}

inline policy& operator>>=(policy& lhs, const policy& rhs)
{ return lhs = lhs >> rhs; }


policy operator+(policy lhs, policy rhs);

inline policy& operator+=(policy& lhs, const policy& rhs)
{ return lhs = lhs + rhs; }

inline
policy operator not(const policy& pol) {
    return Negation{pol};
}

inline 
policy if_then_else(oxm::field<> f, const policy& positive, const policy& negative) {
    return (filter(f) >> positive) + (filter_not(f) >> negative);
}

// This is only for purpose that seq operator must have higher priorty that parallel operator
inline
policy operator|(const policy& lhs, const policy& rhs)
{
    return operator+(lhs, rhs);
}
inline policy& operator|=(policy& lhs, const policy& rhs)
{ return lhs = lhs | rhs; }

// Operators
bool operator==(const Stop&, const Stop&);
bool operator==(const Id&, const Id&);
bool operator==(const Filter& lhs, const Filter& rhs);
bool operator==(const Negation& lhs, const Negation& rhs);
bool operator==(const Modify& lhs, const Modify& rhs);
bool operator==(const PacketFunction& lhs, const PacketFunction& rhs);
bool operator==(const Sequential& lhs, const Sequential& rhs);
bool operator==(const Parallel& lhs, const Parallel& rhs);
bool operator==(const FlowSettings& lhs, const FlowSettings& rhs);

std::ostream& operator<<(std::ostream& out, const Filter& fil);
std::ostream& operator<<(std::ostream& out, const Negation& neg);
std::ostream& operator<<(std::ostream& out, const Stop& stop);
std::ostream& operator<<(std::ostream& out, const Id&);
std::ostream& operator<<(std::ostream& out, const Modify& mod);
std::ostream& operator<<(std::ostream& out, const Sequential& seq);
std::ostream& operator<<(std::ostream& out, const Parallel& par);
std::ostream& operator<<(std::ostream& out, const PacketFunction& func);
std::ostream& operator<<(std::ostream& out, const FlowSettings& flow);
} // namespace retic
} // namespace runos

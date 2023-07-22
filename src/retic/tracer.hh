#pragma once

#include <stdexcept>
#include <functional>
#include <boost/optional.hpp>
#include "policies.hh"
#include "api/Packet.hh"
#include "maple/Tracer.hh"

namespace runos {
namespace retic {
namespace tracer {

struct load_node {
    oxm::field<> field;
};

struct test_node {
    oxm::field<> field;
    bool result;
};


using trace_node = boost::variant<load_node, test_node>;

class Trace: public maple::Tracer {
public:

    using container = std::vector<trace_node>;
    using const_iterator = container::const_iterator;

    policy result() const {
        return m_result;
    }

    const container& values() const {
        return m_trace_impl;
    }

    const_iterator begin() const {return m_trace_impl.begin();}
    const_iterator end() const {return m_trace_impl.end();}

    void setResult(policy result) {
        m_result = result;
    }

    void load(oxm::field<> unexplored) override;

    void test(oxm::field<> pred, bool ret) override;

    maple::Installer finish(maple::FlowPtr flow) override {
        throw std::runtime_error("Not implmented");
    }

    void vload(oxm::field<> by, oxm::field<> mask) override {
        throw std::runtime_error("vload method isn't implemented in retic");
    }
private:
    policy m_result;
    container m_trace_impl;
};

class Tracer {
public:
    Tracer(std::function<policy(Packet&)> p)
        : m_packet_handler(p)
    { }
    Tracer(policy m_policy)
        : m_packet_handler(boost::get<PacketFunction>(m_policy).function)
    { }

    Trace trace(Packet& pkt) const;
private:
    std::function<policy(Packet&)> m_packet_handler;
};

Trace mergeTrace(const std::vector<Trace>& trace, oxm::field_set cache = {});

// For test operators

inline
bool operator==(const load_node& lhs, const load_node& rhs) 
{ return lhs.field == rhs.field; }

inline
bool operator==(const test_node& lhs, const test_node& rhs) 
{ return lhs.field == rhs.field && lhs.result == rhs.result; }

inline
std::ostream& operator<<(std::ostream& out, const load_node& ln) {
    return out << "[ tracer load node : " << ln.field << " ]";
}

inline
std::ostream& operator<<(std::ostream& out, const test_node& tn) {
    return out << "[ tracer load node : " << tn.field << " result: " << tn.result << " ]";
}

} // namespace tracer
} // namespace retic
} // namespace runos

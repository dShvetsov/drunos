#include "policies.hh"

#include <boost/variant/static_visitor.hpp>

namespace runos {
namespace retic {

policy operator+(policy lhs, policy rhs)
{
    if (boost::get<Stop>(&lhs) != nullptr) {
        return rhs; // just ignore left stop policy
    } else if (boost::get<Stop>(&rhs) != nullptr) {
        return lhs; // just ignore right stop polisy
    } else {
        return Parallel{lhs, rhs};
    }
}

// Operators
bool operator==(const Stop&, const Stop&) {
    return true;
}

bool operator==(const Id&, const Id&) {
    return true;
}

bool operator==(const Filter& lhs, const Filter& rhs) {
    return lhs.field == rhs.field;
}

bool operator==(const Negation& lhs, const Negation& rhs) {
    return lhs.pol == rhs.pol;
}

bool operator==(const Modify& lhs, const Modify& rhs) {
    return lhs.field == rhs.field;
}

bool operator==(const PacketFunction& lhs, const PacketFunction& rhs) {
    return lhs.id == rhs.id;
}

bool operator==(const Sequential& lhs, const Sequential& rhs) {
    return lhs.one == rhs.one && lhs.two == rhs.two;
}

bool operator==(const Parallel& lhs, const Parallel& rhs) {
    // TODO: this function needs test. A fully test for such cases (a + b + c) == (b + c + a) == (c + b + a) ... etc
    return (lhs.one == rhs.one && lhs.two == rhs.two) ||
           (lhs.one == rhs.two && lhs.two == rhs.one);
}

bool operator==(const FlowSettings& lhs, const FlowSettings& rhs) {
    return lhs.idle_timeout == rhs.idle_timeout && lhs.hard_timeout == rhs.hard_timeout;
}

std::ostream& operator<<(std::ostream& out, const Filter& fil) {
    return out << "filter(" << fil.field << ")";
}

std::ostream& operator<<(std::ostream& out, const Negation& neg) {
    return out << "not(" << neg.pol << ")";
}

std::ostream& operator<<(std::ostream& out, const Stop& stop) {
    return out << "stop";
}

std::ostream& operator<<(std::ostream& out, const Id&) {
    return out << "id";
}

std::ostream& operator<<(std::ostream& out, const Modify& mod) {
    return out << "modify(" << mod.field << ")";
}

std::ostream& operator<<(std::ostream& out, const Sequential& seq) {
    auto print = [&out](const policy& p ) {
        if (boost::get<Parallel>(&p) != nullptr) {
            // parallel cases. Then wrap into braces
            out << "(" << p << ")";
        } else {
            out << p;
        }
    };
    print(seq.one);
    out << " >> ";
    print(seq.two);
    return out;
}

std::ostream& operator<<(std::ostream& out, const Parallel& par) {
    auto print = [&out](const policy& p ) {
        if (boost::get<Sequential>(&p) != nullptr) {
            // parallel cases. Then wrap into braces
            out << "(" << p << ")";
        } else {
            out << p;
        }
    };
    print(par.one);
    out << " + ";
    print(par.two);
    return out;
}

std::ostream& operator<<(std::ostream& out, const PacketFunction& func) {
    return out << "function id=" << std::hex << func.id;
}

std::ostream& operator<<(std::ostream& out, const FlowSettings& flow) {
    out << "{";
    using secs = std::chrono::seconds;
    if (flow.idle_timeout == duration::max()) {
        out << "permanent";
    } else if (flow.hard_timeout == duration::zero() ) {
        out << "temporal";
    } else {
        out << "idle timout=" << secs(flow.idle_timeout).count() << " s.";
        if (flow.hard_timeout != duration::max()) {
            out << ", hard_timeout=" << secs(flow.hard_timeout).count() << " s.";
        }
    }
    out << "}";
    return out;
}

} // namespace retic
} // namespace runos

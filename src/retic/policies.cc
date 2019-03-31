#include "policies.hh"

namespace runos {
namespace retic {

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
    return out << "filter( " << fil.field << " )";
}

std::ostream& operator<<(std::ostream& out, const Negation& neg) {
    return out << "not( " << neg.pol << " )";
}

std::ostream& operator<<(std::ostream& out, const Stop& stop) {
    return out << "stop";
}

std::ostream& operator<<(std::ostream& out, const Id&) {
    return out << "id";
}

std::ostream& operator<<(std::ostream& out, const Modify& mod) {
    return out << "modify( " << mod.field << " )";
}

std::ostream& operator<<(std::ostream& out, const Sequential& seq) {
    return out << seq.one << " >> " << seq.two;
}

std::ostream& operator<<(std::ostream& out, const Parallel& par) {
    return out << par.one << " + " << par.two;
}

std::ostream& operator<<(std::ostream& out, const PacketFunction& func) {
    return out << " function ";
}

std::ostream& operator<<(std::ostream& out, const FlowSettings& flow) {
    return out
        << "{ idle_timeout = " << std::chrono::seconds(flow.idle_timeout).count() << " sec. "
        << "hard_timeout = " << std::chrono::seconds(flow.hard_timeout).count() << " sec. }";
}

} // namespace retic
} // namespace runos

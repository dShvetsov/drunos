#pragma once

#include <ostream>

#include "policies.hh"

namespace runos {
namespace retic {


// TODO: remove this class
class Dumper : public boost::static_visitor<> {
    public:

    Dumper(std::ostream& os)
        : m_os(os)
    { }

    void operator()(const Filter& fil) {
        m_os << "filter( " << fil.field << " )";
    }

    void operator()(const Stop& stop) {
        m_os << "stop";
    }

    void operator()(const Modify& mod) {
        m_os << "modify( " << mod.field << " )";
    }

    void operator()(const Sequential& seq) {
        boost::apply_visitor(*this, seq.one);
        m_os << " >> ";
        boost::apply_visitor(*this, seq.two);
    }

    void operator()(const Parallel& par) {
        boost::apply_visitor(*this, par.one);
        m_os << " + ";
        boost::apply_visitor(*this, par.two);
    }

    void operator()(const PacketFunction& func) {
        m_os << " function ";
    }
private:
    std::ostream& m_os;
};


} // namespace retic
} // namespace runos

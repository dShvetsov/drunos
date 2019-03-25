#pragma once

#include <boost/variant/static_visitor.hpp>

#include "fdd.hh"
#include "api/Packet.hh"

namespace runos {
namespace retic{
namespace fdd {
class Traverser: public boost::static_visitor<leaf&> {
public:
    Traverser(const Packet& pkt): m_pkt(pkt) { }
    leaf& operator()(leaf& l);
    leaf& operator()(node& n);
private:
    const Packet& m_pkt;
};

} // fdd
} // retic
} // runos

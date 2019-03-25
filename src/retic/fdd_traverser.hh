#pragma once

#include <boost/static_visitor.hpp>

#include "fdd.hh"
#include "api/Packet.hh"

namespace runos {
namespace retic {
namespace fdd {

class FddTraveser : public boost::static_visitor {
    FddTraveser(const Packet& pkt): m_pkt(pkt)
    { }

    leaf& operator()(const node& n);
    leaf& operator()(const leaf& l);
private:
    const Packet& m_pkt;
};

} // namespace runos
} // namespace retic
} // namespace fdd

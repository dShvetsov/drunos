#pragma once

#include <boost/variant/static_visitor.hpp>

#include "fdd.hh"
#include "backend.hh"
#include "api/Packet.hh"

namespace runos {
namespace retic{
namespace fdd {
class Traverser: public boost::static_visitor<leaf&> {
public:
    Traverser(const Packet& pkt, Backend* backend = nullptr)
        : m_pkt(pkt)
        , m_backend(backend)
    { }
    leaf& operator()(leaf& l);
    leaf& operator()(node& n);
private:
    const Packet& m_pkt;
    Backend* m_backend;

};

} // fdd
} // retic
} // runos

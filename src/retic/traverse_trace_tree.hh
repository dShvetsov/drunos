#pragma once

#include <boost/variant/static_visitor.hpp>

#include "trace_tree.hh"

namespace runos {
namespace retic {
namespace trace_tree {

class Traverser: public boost::static_visitor<std::shared_ptr<fdd::diagram_holder>>
{
public:
    Traverser(const Packet& pkt): m_pkt(pkt) { }
    std::shared_ptr<fdd::diagram_holder> operator()(unexplored& n);
    std::shared_ptr<fdd::diagram_holder> operator()(leaf_node& n);
    std::shared_ptr<fdd::diagram_holder> operator()(load_node& ln);
    std::shared_ptr<fdd::diagram_holder> operator()(test_node& tn);

private:
    const Packet& m_pkt;
};

}
}
}

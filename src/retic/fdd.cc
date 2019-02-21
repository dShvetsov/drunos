#include "fdd.hh"

#include <exception>
#include <iterator>

#include <oxm/openflow_basic.hh>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/multivisitors.hpp>

#include "policies.hh"

namespace runos {
namespace retic {
namespace fdd {

struct parallel_compositon: public boost::static_visitor<diagram>
{
    diagram operator()(const leaf& lhs, const leaf& rhs) {
        leaf ret;
        ret.sets.reserve(lhs.sets.size() + rhs.sets.size());
        ret.sets.insert(ret.sets.end(), lhs.sets.begin(), lhs.sets.end());
        ret.sets.insert(ret.sets.end(), rhs.sets.begin(), rhs.sets.end());
        return ret;
    }
    diagram operator()(const leaf&, const node&) {
        throw std::runtime_error("Not implemented");
    }
    diagram operator()(const node&, const leaf&) 
        { throw std::runtime_error("Not implemented"); }
    diagram operator()(const node&, const node&) 
         {throw std::runtime_error("Not implemented"); }
};

diagram compile(const policy& p) {
    Compiler compiler;
    return boost::apply_visitor(compiler, p);
}

diagram Compiler::operator()(const Filter& fil) {
    return node{fil.field, leaf{{oxm::field_set()}}, leaf{}};
}

diagram Compiler::operator()(const Modify& mod) {
    return leaf{{oxm::field_set{mod.field}}};
}
diagram Compiler::operator()(const Stop& stop) {
    return leaf{};
}
diagram Compiler::operator()(const Sequential&) {
    throw std::runtime_error("Not implemented");
}
diagram Compiler::operator()(const Parallel& p) {
    parallel_compositon dispatcher;
    diagram d1 = boost::apply_visitor(*this, p.one);
    diagram d2 = boost::apply_visitor(*this, p.two);
    return boost::apply_visitor(dispatcher, d1, d2);
}
diagram Compiler::operator()(const PacketFunction&) {
    throw std::runtime_error("Not implemented");
}

} // namespace fdd
} // namespace retic
} // namespace runos

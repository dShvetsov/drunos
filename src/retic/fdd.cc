#include "fdd.hh"

#include <exception>

#include <oxm/openflow_basic.hh>

#include <boost/variant/static_visitor.hpp>

#include "policies.hh"

namespace runos {
namespace retic {
namespace fdd {


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
diagram Compiler::operator()(const Parallel&) {
    throw std::runtime_error("Not implemented");
}
diagram Compiler::operator()(const PacketFunction&) {
    throw std::runtime_error("Not implemented");
}

} // namespace fdd
} // namespace retic
} // namespace runos

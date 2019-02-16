#include "fdd.hh"

#include "policies.hh"

#include <oxm/openflow_basic.hh>
#include <oxm/field.hh>
#include <oxm/field_set.hh>

#include <boost/variant/static_visitor.hpp>

namespace runos {
namespace retic {

struct fdd::node {
    oxm::field<> field;
    diagram positive;
    diagram negative;
};

struct fdd::compiler: boost::static_visitor<> {
    diagram operator()(const Filter& fil) {
        return fdd::node{fil.field, fdd::leaf{false}, fdd::leaf{true}};
    }
};

} // namespace retic
} // namespace runos

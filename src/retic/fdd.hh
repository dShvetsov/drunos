#pragma once

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>

#include <oxm/field.hh>
#include <oxm/field_set.hh>

#include "policies.hh"

namespace runos {
namespace retic {
namespace fdd {

struct leaf {
    std::vector<oxm::field_set> sets;
};

struct node;
struct compiler;

using diagram = boost::variant<
        leaf,
        boost::recursive_wrapper<node>
    >;


struct node {
    oxm::field<> field;
    diagram positive;
    diagram negative;
};

} // namespace fdd
} // namespace retic
} // namespace runos

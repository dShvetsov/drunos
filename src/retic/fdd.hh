#pragma once

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>

namespace runos {
namespace retic {

class fdd {
private:
    struct leaf { bool value; };
    struct node;
    struct compiler;

    using diagram = boost::variant<
            leaf,
            boost::recursive_wrapper<node>
        >;
};

} // namespace retic
} // namespace runos

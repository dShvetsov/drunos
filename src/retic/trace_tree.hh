#pragma once

// TODO: Try to reuse maple/TraceTree.hh

#include <exception>
#include <unordered_map>
#include <ostream>

#include <boost/variant/variant_fwd.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>

#include "oxm/field.hh"

#include "tracer.hh"
#include "policies.hh"

namespace runos {
namespace retic {
namespace trace_tree {

struct unexplored {};

struct leaf_node {
    policy p;
};

struct test_node;
struct load_node;

using node = boost::variant<
        unexplored,
        leaf_node,
        boost::recursive_wrapper<test_node>,
        boost::recursive_wrapper<load_node>
    >;

struct test_node {
    oxm::field<> need;
    node positive;
    node negative;
};

struct load_node {
    oxm::mask<> mask;
    std::unordered_map<bits<>, node> cases;
};

class Augmention : public boost::static_visitor<> {
public:
    struct inconsistent_trace: public std::exception { };
    Augmention(node* root)
        : current(root)
    { }

    void operator()(tracer::load_node& ln);
    void operator()(tracer::test_node& tn);
    void finish(policy pol);

private:
    node* current;
};

std::ostream& operator<<(std::ostream& out, const unexplored& u);
std::ostream& operator<<(std::ostream& out, const leaf_node& l);
std::ostream& operator<<(std::ostream& out, const test_node& t);
std::ostream& operator<<(std::ostream& out, const load_node& l);

bool operator==(const unexplored& lhs, const unexplored& rhs);
bool operator==(const leaf_node& lhs, const leaf_node& rhs);
bool operator==(const test_node& lhs, const test_node& rhs);
bool operator==(const load_node& lhs, const load_node& rhs);

} // namespace trace_tree
} // namespace retic
} // namespace runos

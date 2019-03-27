#pragma once

// TODO: Try to reuse maple/TraceTree.hh

#include <exception>
#include <memory>
#include <unordered_map>
#include <ostream>

#include <boost/variant/variant_fwd.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>

#include "oxm/field.hh"
#include "oxm/field_set.hh"

#include "tracer.hh"
#include "backend.hh"
#include "policies.hh"

namespace runos {
namespace retic {

// forward declaration
// TODO: UnhackMe
namespace fdd {
    struct diagram_holder;
}

namespace trace_tree {

struct unexplored {};

struct leaf_node {
    policy p;
    std::shared_ptr<fdd::diagram_holder> kat_diagram;
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
        , m_backend(nullptr)
    { }

    Augmention(node* root, Backend* backend, oxm::field_set pre_match, uint16_t prio_down, uint16_t prio_up)
        : current(root)
        , m_backend(backend)
        , match(pre_match)
        , prio_down(prio_down)
        , prio_up(prio_up)
    { }

    void operator()(const tracer::load_node& ln);
    void operator()(const tracer::test_node& tn);
    std::shared_ptr<fdd::diagram_holder> finish(policy pol);

private:
    node* current;
    Backend* m_backend;
    oxm::field_set match;
    uint16_t prio_down;
    uint16_t prio_up;
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

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

class restriction {
public:
    oxm::field<> field;
    diagram d;
    bool test; // positive or negative test

    diagram apply();
};

class Compiler: public boost::static_visitor<diagram> {
public:
    diagram operator()(const Filter& fil) const;
    diagram operator()(const Modify& mod) const;
    diagram operator()(const Stop& stop) const;
    diagram operator()(const Sequential&) const;
    diagram operator()(const Parallel&) const;
    diagram operator()(const PacketFunction&) const;
};

diagram compile(const policy&);

bool operator==(const leaf& lhs, const leaf& rhs);
bool operator==(const node& lhs, const node& rhs);
std::ostream& operator<<(std::ostream& out, const leaf& v);
std::ostream& operator<<(std::ostream& out, const node& v);
int compare_types(const oxm::type lhs, oxm::type rhs);

struct parallel_composition: public boost::static_visitor<diagram>
{
    diagram operator()(const leaf& lhs, const leaf& rhs) const;
    diagram operator()(const node& lhs, const leaf& rhs) const;
    diagram operator()(const leaf& lhs, const node& rhs) const;
    diagram operator()(const node&, const node&) const;
};

struct sequential_composition: public boost::static_visitor<diagram>
{
    diagram operator()(const leaf& lhs, const diagram& rhs) const;
    diagram operator()(const node& lhs, const leaf& rhs) const;
    diagram operator()(const node&, const node&) const;
private:
    struct left_action_applier: public boost::static_visitor<diagram>
    {
        left_action_applier(oxm::field_set action): action(action)
        { }
        oxm::field_set action;

        diagram operator()(const leaf& l) const;
        diagram operator()(const node& n) const;
    };
};

} // namespace fdd
} // namespace retic
} // namespace runos

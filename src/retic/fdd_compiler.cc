#include "fdd_compiler.hh"

#include <exception>
#include <iterator>

#include <oxm/openflow_basic.hh>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/multivisitors.hpp>

#include "policies.hh"

namespace runos {
namespace retic {
namespace fdd {

diagram compile(const policy& p) {
    Compiler compiler;
    return boost::apply_visitor(compiler, p);
}

diagram Compiler::operator()(const Filter& fil) const {
    node ret{
        oxm::mask<>(fil.field),
        {},
        leaf{}
    };
    auto s = ret.cases.emplace(fil.field..value_bits(), leaf{{oxm::field_set{}}});
    ASSERT(s.second);
    return ret;
}

diagram Compiler::operator()(const Modify& mod) const {
    return leaf{{oxm::field_set{mod.field}}};
}
diagram Compiler::operator()(const Stop& stop) const {
    return leaf{};
}
diagram Compiler::operator()(const Sequential& s) const {
    sequential_composition dispatcher;
    diagram d1 = boost::apply_visitor(*this, s.one);
    diagram d2 = boost::apply_visitor(*this, s.two);
    return boost::apply_visitor(dispatcher, d1, d2);
}
diagram Compiler::operator()(const Parallel& p) const {
    parallel_composition dispatcher;
    diagram d1 = boost::apply_visitor(*this, p.one);
    diagram d2 = boost::apply_visitor(*this, p.two);
    return boost::apply_visitor(dispatcher, d1, d2);
}
diagram Compiler::operator()(const PacketFunction& f) const {
    return leaf{ {{oxm::field_set{}, f}} };
}


// ================= Compositions operators ============================

// ---- Parallel -----

static diagram default_parallel_leaf_operation(const leaf& lhs, diagram& rhs) {
    node ret{lhs.mask, {}, leaf{}};
    for (auto& [bits, d]: lhs.cases) {
        auto s = ret.cases.emplace(bits, boost::apply_visitor(*this, d, rhs));
        ASSERT(s.second);
    }
    ret.otherwise = boost::apply_visitor(*this, lhs.otherwise, rhs);
    return ret;
}

diagram parallel_composition::operator()(const leaf& lhs, const leaf& rhs) const {
    leaf ret;
    ret.sets.reserve(lhs.sets.size() + rhs.sets.size());
    ret.sets.insert(ret.sets.end(), lhs.sets.begin(), lhs.sets.end());
    ret.sets.insert(ret.sets.end(), rhs.sets.begin(), rhs.sets.end());
    return ret;
}

diagram parallel_composition::operator()(const node& lhs, const leaf& rhs) const {
    return default_parallel_leaf_operation(lhs, rhs);
}


diagram parallel_composition::operator()(const leaf& lhs, const node& rhs) const {
    return boost::apply_visitor(*this, diagram{rhs}, diagram{lhs});
}

diagram parallel_composition::operator()(const node& lhs, const node& rhs) const {
    if (lhs.mask.type() == rhs.mask.type()) {
        node ret {lhs.mask, {}, leaf{}};
        for (auto& [bits, d]: lhs.cases) {
            auto it = rhs.cases.find(bits);
            if (it != rhs.cases.end()) {
                auto s = ret.cases.emplace(bits, boost::apply_visitor(*this, d, it->second));
                ASSERT(s.second);
            } else {
                auto s = ret.cases.emplace(bits, boost::apply_visitor(*this, d, rhs.otherwise));
                ASSERT(s.second);
            }
        }
        for (auto& [bits, d]: rhs.cases) {
            auto it = lhs.cases.find(bits);
            if (it != lhs.cases.end()) {
                // Skip this part becouse already added
            } else {
                auto s = ret.cases.emplace(bits, boost::apply_visitor(*this, d, lhs.otherwise));
                ASSERT(s.second);
            }
        }
        ret.otherwise = boost::apply_visitor(*this, lhs.otherwise, rhs.otherwise);
    } else if (compare_types(lhs.mask.type(), rhs.mask.type()) > 0) {
        return default_parallel_leaf_operation(lhs, rhs);
    } else {
        return default_parallel_leaf_operation(rhs, lhs);
    }
    throw std::logic_error("Unexsting case of ordering fields");
}


// ----Sequential----

diagram sequential_composition::operator()(const leaf& lhs, const diagram& rhs) const
{
    if (lhs.sets.empty()) {
        return leaf{};
    }

    diagram result = leaf{};
    parallel_composition parallel;
    for (auto& action: lhs.sets) {
        left_action_applier applier{action};
        diagram current = boost::apply_visitor(applier, rhs);
        result = boost::apply_visitor(parallel, result, current);
    }
    return result;
}

diagram sequential_composition::operator()(const node& lhs, const diagram& rhs) const
{
    diagram ret = boost::

    auto type = lhs.mask.type();
    for (auto& [bits, d]: lhs.cases) {
        diagram child_d = boost::apply_visitor(*this, d, rhs);
        oxm::field<> f = (type == bits) & lhs.mask;
        auto r = restriction(f, child_d, true);
        auto s = ret.cases.emplace(bits, r.apply());
        ASSERT(s.second);
    }
    diagram child_d = boost::apply_visitor(*this, otherwise, rhs);
    oxm::field<> f = (type == bits) & lhs.mask;
    auto r = restriction{f, child_d, false};
    ret.otherwise = 

    diagram one = boost::apply_visitor(*this, lhs.positive, rhs);
    diagram two = boost::apply_visitor(*this, lhs.negative, rhs);
    diagram one_restricted = restriction{lhs.field, one, true}.apply();
    diagram two_restricted = restriction{lhs.field, two, false}.apply();
    parallel_composition parallel;
    return boost::apply_visitor(parallel, one_restricted, two_restricted);
}

static oxm::field_set field_set_union(const oxm::field_set& lhs, const oxm::field_set rhs) {
    oxm::field_set ret_value = lhs;
    for (auto& v : rhs) {
        ret_value.modify(v);
    }
    return ret_value;
}

static action_unit seq_actions(const action_unit& one, const action_unit& two) {
    if (one.body.has_value()) {
        action_unit emty;
        action_unit& passed_value = one.post_actions == nullptr ? emty : *one.post_actions;
        return action_unit(one.pred_actions, one.body.value(), seq_actions(passed_value, two));
    } else {
        return action_unit(field_set_union(one.pred_actions, two.pred_actions), two.body, two.post_actions);
    }
}

diagram sequential_composition::left_action_applier::operator()(const leaf& l) const {
    leaf result{};
    result.sets.reserve(l.sets.size());
    for (auto& a : l.sets) {
        action_unit au = seq_actions(action, a);
        result.sets.push_back(au);
    }
    return result;
}

diagram sequential_composition::left_action_applier::operator()(const node& n) const {
    auto it = action.pred_actions.find(n.field.type());
    if (it == action.pred_actions.end()) {
        diagram positive = boost::apply_visitor(*this, n.positive);
        diagram negative = boost::apply_visitor(*this, n.negative);
        return node{n.field, positive, negative};
    }
    else if (*it == n.field) {
        return boost::apply_visitor(*this, n.positive);
    } else {
        // have this type, but other value
        return boost::apply_visitor(*this, n.negative);
    }
}


// ----restriction operation----//


diagram restriction::apply() {
    struct applier_true : public boost::static_visitor<diagram>
    {
        applier_true(oxm::field<> f) : f(f) { }

        diagram operator()(const leaf& l) const {
            return node{f, l, leaf{}};
        }

        diagram operator()(const node& n) const {
            if (n.field == f) {
                return node{f, n.positive, leaf{}};
            } else if (n.field.type() == f.type()) {
                return boost::apply_visitor(*this, n.negative);
            } else if (compare_types(f.type(), n.field.type()) > 0) {
                return node{f, n, leaf{}};
            } else {
                diagram positive = boost::apply_visitor(*this, n.positive);
                diagram negative = boost::apply_visitor(*this, n.negative);
                return node{n.field, positive, negative};
            }
        }
        const oxm::field<> &f;
    };

    struct applier_false : public boost::static_visitor<diagram>
    {
        applier_false(oxm::field<> f) : f(f) { }

        diagram operator()(const leaf& l) const {
            return node{f, leaf{}, l};
        }

        diagram operator()(const node& n) const {
            if (n.field == f) {
                return node{f, leaf{}, n.negative};
            } else if (n.field.type() == f.type()) {
                if (n.field.value_bits() < f.value_bits()) {
                    return node{n.field, n.positive, boost::apply_visitor(*this, n.negative)};
                } else {
                    return node{f, leaf{}, n};
                }
            } else if (compare_types(f.type(), n.field.type()) > 0) {
                return node{f, leaf{}, n};
            } else {
                diagram positive = boost::apply_visitor(*this, n.positive);
                diagram negative = boost::apply_visitor(*this, n.negative);
                return node{n.field, positive, negative};
            }
        }
        const oxm::field<> &f;
    };
    if (test) {
        return boost::apply_visitor(applier_true(field), d);
    } else {
        return boost::apply_visitor(applier_false(field), d);
    }
}

//=============== Operators =======================//

bool operator==(const node& lhs, const node& rhs) {
    return lhs.field == rhs.field &&
        lhs.positive == rhs.positive && lhs.negative == rhs.negative;
}


bool operator==(const leaf& lhs, const leaf& rhs) {
    for (const auto& f: lhs.sets) {
        if (std::find(rhs.sets.begin(), rhs.sets.end(), f) == rhs.sets.end()) {
            return false;
        }
    }
    for (const auto& f: rhs.sets) {
        if (std::find(lhs.sets.begin(), lhs.sets.end(), f) == lhs.sets.end()) {
            return false;
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& out, const node& rhs) {
    out << "field " << rhs.field << " ? (" << rhs.positive << ") : (" << rhs.negative << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, const leaf& rhs) {
    out << "set_field: [ ";
    for (auto& i: rhs.sets) {
        out << "(" << i.pred_actions << ") ";
    }
    out << "]";
    return out;
}

int compare_types(const oxm::type lhs_type, oxm::type rhs_type)
{
    uint64_t lhs_type_id = lhs_type.ns() << 16 | lhs_type.id();
    uint64_t rhs_type_id = rhs_type.ns() << 16 | rhs_type.id();
    return rhs_type_id - lhs_type_id;
}

} // namespace fdd
} // namespace retic
} // namespace runos

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


diagram compile(const policy& p) {
    Compiler compiler;
    return boost::apply_visitor(compiler, p);
}

diagram Compiler::operator()(const Filter& fil) const {
    return node{fil.field, leaf{{oxm::field_set()}}, leaf{}};
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
diagram Compiler::operator()(const PacketFunction&) const {
    throw std::runtime_error("Not implemented");
}


// ================= Compositions operators ============================

// ---- Parallel -----
diagram parallel_composition::operator()(const leaf& lhs, const leaf& rhs) const {
    leaf ret;
    ret.sets.reserve(lhs.sets.size() + rhs.sets.size());
    ret.sets.insert(ret.sets.end(), lhs.sets.begin(), lhs.sets.end());
    ret.sets.insert(ret.sets.end(), rhs.sets.begin(), rhs.sets.end());
    return ret;
}

diagram parallel_composition::operator()(const node& lhs, const leaf& rhs) const {
    diagram positive = boost::apply_visitor(*this, lhs.positive, diagram{rhs});
    diagram negative = boost::apply_visitor(*this, lhs.negative, diagram{rhs});
    return node{lhs.field, positive, negative};
}

diagram parallel_composition::operator()(const leaf& lhs, const node& rhs) const {
    return boost::apply_visitor(*this, diagram{rhs}, diagram{lhs});
}

diagram parallel_composition::operator()(const node& lhs, const node& rhs) const {
    if (lhs.field == rhs.field) {
        diagram positive = boost::apply_visitor(
            *this, diagram(lhs.positive), diagram(rhs.positive)
        );
        diagram negative = boost::apply_visitor(
            *this, diagram(lhs.negative), diagram(rhs.negative)
        );
        return node{lhs.field, positive, negative};
    } else if (lhs.field.type() == rhs.field.type()) {
        if (lhs.field.value_bits() < rhs.field.value_bits()) {
            diagram positive = boost::apply_visitor(
                *this, diagram(lhs.positive), diagram(rhs.negative)
            );
            diagram negative = boost::apply_visitor(
                *this, diagram(lhs.negative), diagram(rhs)
            );
            return node{lhs.field, positive, negative};
        } else {
            return boost::apply_visitor(
                *this, diagram(rhs), diagram(lhs)
            );
        }
    } else if (compare_types(lhs.field.type(), rhs.field.type()) > 0) {
        diagram positive = boost::apply_visitor(*this, diagram(lhs.positive), diagram(rhs));
        diagram negative = boost::apply_visitor(*this, diagram(lhs.negative), diagram(rhs));
        return node{lhs.field, positive, negative};
    } else {
        return boost::apply_visitor(
            *this, diagram(rhs), diagram(lhs)
        );
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

diagram sequential_composition::operator()(const node& lhs, const leaf& rhs) const
{
    throw std::runtime_error("Not implemented");
}

diagram sequential_composition::operator()(const node& lhs, const node& rhs) const
{
    throw std::runtime_error("Not implemented");
}

diagram sequential_composition::left_action_applier::operator()(const leaf& l) const {
    leaf result{};
    result.sets.reserve(l.sets.size());
    for (auto& a : l.sets) {
        oxm::field_set fs = action;
        for (auto& f: a) { fs.modify(f); }
        result.sets.push_back(fs);
    }
    return result;
}

diagram sequential_composition::left_action_applier::operator()(const node& n) const {
    throw std::runtime_error("Not implemented");
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
        out << "(" << i << ") ";
    }
    out << " ]";
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

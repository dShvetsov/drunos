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
    parallel_composition dispatcher;
    diagram d1 = boost::apply_visitor(*this, p.one);
    diagram d2 = boost::apply_visitor(*this, p.two);
    return boost::apply_visitor(dispatcher, d1, d2);
}
diagram Compiler::operator()(const PacketFunction&) {
    throw std::runtime_error("Not implemented");
}


// ================= Compositions operators ============================

diagram parallel_composition::operator()(const leaf& lhs, const leaf& rhs) {
    leaf ret;
    ret.sets.reserve(lhs.sets.size() + rhs.sets.size());
    ret.sets.insert(ret.sets.end(), lhs.sets.begin(), lhs.sets.end());
    ret.sets.insert(ret.sets.end(), rhs.sets.begin(), rhs.sets.end());
    return ret;
}

diagram parallel_composition::operator()(const node& lhs, const leaf& rhs) {
    diagram positive = boost::apply_visitor(*this, lhs.positive, diagram{rhs});
    diagram negative = boost::apply_visitor(*this, lhs.negative, diagram{rhs});
    return node{lhs.field, positive, negative};
}

diagram parallel_composition::operator()(const leaf& lhs, const node& rhs) {
    return boost::apply_visitor(*this, diagram{rhs}, diagram{lhs});
}

diagram parallel_composition::operator()(const node& lhs, const node& rhs) {
    if (lhs.field == rhs.field) {
        diagram positive = boost::apply_visitor(
            *this, diagram(lhs.positive), diagram(rhs.positive)
        );
        diagram negative = boost::apply_visitor(
            *this, diagram(lhs.negative), diagram(rhs.negative)
        );
        return node{lhs.field, positive, negative};
    } else if (lhs.field.type() == rhs.field.type()) {
        diagram positive = boost::apply_visitor(
            *this, diagram(lhs.positive), diagram(rhs.negative)
        );
        diagram negative = boost::apply_visitor(
            *this, diagram(lhs.negative), diagram(rhs)
        );
        return node{lhs.field, positive, negative};
    }
    return leaf{};
}


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

} // namespace fdd
} // namespace retic
} // namespace runos

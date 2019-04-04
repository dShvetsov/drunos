#include "dotdumper.hh"

#include <string>
#include <algorithm>

#include <boost/variant/static_visitor.hpp>
#include <boost/lexical_cast.hpp>

#include "fdd_compiler.hh"


namespace runos {
namespace retic {


std::string get_id() {
    static uint64_t id = 0;
    id++;
    return boost::lexical_cast<std::string>(id);
}

namespace fdd {

class DumpFdd: public boost::static_visitor<std::string> {
public:
    DumpFdd(std::ostream& out) : out(out) { }

    std::string operator()(const leaf& l) const;
    std::string operator()(const node& n) const;
    std::ostream& out;
};

} // namespace fdd

namespace trace_tree {
class DumpTraceTree: public boost::static_visitor<std::string> {
public:
    DumpTraceTree(std::ostream& out) : out(out) { }

    std::string operator()(const leaf_node& l) const {
        auto id = get_id();
        out << id << "[shape=box label=\"" << l.p << "\"];\n";
        if (l.kat_diagram != nullptr) {
            auto child_id = boost::apply_visitor(fdd::DumpFdd(out), l.kat_diagram->value);
            out << id << " -> " << child_id << ";\n";
        }
        return id;
    }

    std::string operator()(const load_node& n) const {
        auto id = get_id();

        out << id << "[label=\"";
        if (n.mask.exact()) {
            out << n.mask.type();
        } else {
            out << n.mask;
        }
        out << "\"];\n";

        std::string child_id;
        for (auto& [bits, next_n]: n.cases) {
            child_id = boost::apply_visitor(*this, next_n);
            out << id << " -> " << child_id << "[label=\"";
            n.mask.type().print(out, bits);
            out << "\", color=green];\n";
        }
        return id;
    }

    std::string operator()(const test_node& n) const {
        auto id = get_id();
        out << id << "[label=\"" << n.need << "\"];\n";
        std::string child_id;
        child_id = boost::apply_visitor(*this, n.positive);
        out << id << " -> " << child_id << "[label=\"+\", color=green];\n";
        child_id = boost::apply_visitor(*this, n.negative);
        out << id << " -> " << child_id << "[label=\"-\", color=red];\n";
        return id;
    }

    std::string operator()(const unexplored& n) const {
        auto id = get_id();
        out << id << "[label=\"unexplored\"];\n";
        return id;
    }

    std::ostream& out;
};
}


namespace fdd {

std::string DumpFdd::operator()(const leaf& l) const {
    auto id = get_id();
    if (std::none_of(
            l.sets.begin(), l.sets.end(),
            [](auto& x){ return x.body.has_value(); }
    )) {
        out << id << "[shape=box, label=\"" << l << "\"]\n";
        return id;
    } else {
        out << id << "[shape=box, label=fdd_leaf]";
        auto child_id = boost::apply_visitor(trace_tree::DumpTraceTree(out), l.maple_tree);
        out << id << " -> " << child_id << ";\n";
        return id;
    }
}

std::string DumpFdd::operator()(const node& n) const {
    auto id = get_id();
    out << id << "[label=\"" << n.field << "\"];\n";
    std::string child_id;
    child_id = boost::apply_visitor(*this, n.positive);
    out << id << " -> " << child_id << "[label=\"+\", color=green]";
    child_id = boost::apply_visitor(*this, n.negative);
    out << id << " -> " << child_id << "[label=\"-\", color=red]";
    return id;
}

} // namespace fdd


void dumpAsDot(const fdd::diagram& d, std::ostream& out) {
    out << "digraph G {\n";
    boost::apply_visitor(fdd::DumpFdd(out), d);

    out << "}\n";
}

}
}

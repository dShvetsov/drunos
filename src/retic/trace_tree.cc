#include "trace_tree.hh"

#include "fdd.hh"
#include "fdd_compiler.hh"
#include "fdd_translator.hh"

namespace runos {
namespace retic {
namespace trace_tree {

void Augmention::operator()(const tracer::load_node& ln) {
    if (boost::get<unexplored>(current)) {
        *current = load_node{oxm::mask<>(ln.field), {} };
        current = &boost::get<load_node>(current)->cases
                  .emplace(ln.field.value_bits(), unexplored{})
                  .first->second; // inserter value
    }
    else if (load_node* load = boost::get<load_node>(current); load != nullptr) {
        if (load->mask != oxm::mask<>(ln.field)) {
            throw inconsistent_trace();
        }
        current = &load->cases[ln.field.value_bits()];
    } else {
        throw inconsistent_trace();
    }
    match.modify(ln.field);
}

void Augmention::operator()(const tracer::test_node& tn) {
    uint16_t prio_middle = (prio_up + prio_down) / 2;
    if (boost::get<unexplored>(current)) {
        *current = test_node{tn.field, unexplored{}, unexplored{}};
        current = tn.result ? &boost::get<test_node>(current)->positive :
                              &boost::get<test_node>(current)->negative ;
        if (m_backend) {
            // install barrier rule
            oxm::field_set barrier_match = match;
            barrier_match.modify(tn.field);
            m_backend->installBarrier(barrier_match, prio_middle);
        }
    } else if (test_node* test = boost::get<test_node>(current); test != nullptr) {
        if (test->need != tn.field) {
            throw inconsistent_trace();
        }
        current = tn.result ? &test->positive : &test->negative;
    }
    if (tn.result) {
        match.modify(tn.field);
        prio_down = prio_middle + 1;
    } else {
        prio_up = prio_middle - 1;
    }
}

void Augmention::finish(policy p) {
    if (boost::get<unexplored>(current)) {
        *current = leaf_node{p};
    } else if (leaf_node* leaf = boost::get<leaf_node>(current); leaf != nullptr) {
        leaf->p = p;
    } else {
        throw inconsistent_trace();
    }

    leaf_node* leaf = boost::get<leaf_node>(current);
    leaf->kat_diagram = std::make_shared<fdd::diagram_holder>();
    leaf->kat_diagram->value = fdd::compile(p);
    if (m_backend) {
        fdd::Translator translator{*m_backend, match, prio_down, prio_up};
        boost::apply_visitor(translator, leaf->kat_diagram->value);
    }
}

std::ostream& operator<<(std::ostream& out, const unexplored& u) {
    return  out << "unexplored";
}

std::ostream& operator<<(std::ostream& out, const leaf_node& l) {
    return  out << "{ leaf " << l.p << "}";
}

std::ostream& operator<<(std::ostream& out, const test_node& t) {
    return  out << "{ test: positive: " << t.need << " " << t.positive << " negative: " << t.negative << "}";
}

std::ostream& operator<<(std::ostream& out, const load_node& l) {
    out << "{ load: " << l.mask << " ";
    for (auto& [bits, n]: l.cases) {
        out << bits << ": " << n;
    }
    out << " }";
    return out;
}

bool operator==(const unexplored& lhs, const unexplored& rhs) {
    return true;
}

bool operator==(const leaf_node& lhs, const leaf_node& rhs) {
    return lhs.p == rhs.p;
}

bool operator==(const test_node& lhs, const test_node& rhs) {
    return lhs.need == rhs.need && lhs.positive == rhs.positive && lhs.negative == rhs.negative;
}

bool operator==(const load_node& lhs, const load_node& rhs) {
    if (lhs.mask != rhs.mask) {
        return false;
    }
    for (auto& [bits, n]: lhs.cases) {
        auto it = rhs.cases.find(bits);
        if (it == rhs.cases.end() || it->second != n) {
            return false;
        }
    }
    for (auto& [bits, n]: rhs.cases) {
        auto it = lhs.cases.find(bits);
        if (it == lhs.cases.end() || it->second != n) {
            return false;
        }
    }
    return true;
}

} // namespace trace_tree
} // namespace retic
} // namespace runos

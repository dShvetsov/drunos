#include "trace_tree_translator.hh"

#include <exception>

namespace runos {
namespace retic {
namespace trace_tree {

void Translator::operator()(const unexplored& u) {
    // do nothing
}

void Translator::operator()(const leaf_node& ln) {
    throw std::runtime_error("Translator::operator()(cnst leaf_node& ) not implemented yet");
}

void Translator::operator()(const test_node& tn) {
    uint16_t save_prio = prio_up;
    uint16_t prio_middle = (prio_up + prio_down) / 2;
    prio_up = prio_middle - 1;
    boost::apply_visitor(*this, tn.negative);
    match.modify(tn.need);
    m_backend.installBarrier(match, prio_middle);
    prio_up = save_prio;
    prio_down = prio_middle + 1;
    boost::apply_visitor(*this, tn.positive);
    match.erase(oxm::mask<>(tn.need));

}

void Translator::operator()(const load_node& load) {
    auto type = load.mask.type();

    for (auto& record: load.cases) {
        match.modify((type == record.first) & load.mask);
        boost::apply_visitor(*this, record.second);
        match.erase(load.mask);
    }
}

} // namespace trace_tree
} // namespace retic
} // namespace runos

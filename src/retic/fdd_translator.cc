#include "fdd_translator.hh"

namespace runos {
namespace retic {
namespace fdd {

void Translator::operator()(const node& n) {
    boost::apply_visitor(*this, n.negative);
    match.modify(n.field);
    boost::apply_visitor(*this, n.positive);
    match.erase(oxm::mask<>(n.field));
}

void Translator::operator()(const leaf& l) {
    std::vector<oxm::field_set> sets;
    sets.reserve(l.sets.size());
    for (auto& s: l.sets) {
        sets.push_back(s.pred_actions);
    }
    m_backend.install(match, sets, priority);
    priority++;
}

} // namespace fdd
} // namespace retic
} // namespace runos

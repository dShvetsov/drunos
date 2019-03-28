#include "fdd_translator.hh"

namespace runos {
namespace retic {
namespace fdd {

void Translator::operator()(const node& n) {

    struct {
        uint16_t prio_up;
        uint16_t prio_middle;
        uint16_t prio_down;
        std::optional<oxm::mask<>> previous_mask;
    } saved_state;

    saved_state.prio_up = prio_up;
    saved_state.prio_down = prio_down;
    saved_state.prio_middle = prio_middle;
    saved_state.previous_mask = previous_mask;

    if (previous_mask.has_value()) {
        if (previous_mask.value() == oxm::mask<>(n.field)) {
            boost::apply_visitor(*this, n.negative);

            prio_down = prio_middle;
            previous_mask = std::nullopt;
            match.modify(n.field);
            boost::apply_visitor(*this, n.positive);
            match.erase(oxm::mask<>(n.field));
        } else {
            prio_up = prio_middle;

            prio_middle = prio_up / 2 + prio_down / 2;
            previous_mask = oxm::mask<>(n.field);
            boost::apply_visitor(*this, n.negative);

            prio_down = prio_middle;
            previous_mask = std::nullopt;
            match.modify(n.field);
            boost::apply_visitor(*this, n.positive);
            match.erase(oxm::mask<>(n.field));
        }
    } else {
        prio_middle = prio_up / 2 + prio_down / 2;
        previous_mask = oxm::mask<>(n.field);
        boost::apply_visitor(*this, n.negative);

        prio_down = prio_middle;
        previous_mask = std::nullopt;
        match.modify(n.field);
        boost::apply_visitor(*this, n.positive);
        match.erase(oxm::mask<>(n.field));
    }

    prio_up = saved_state.prio_up;
    prio_down = saved_state.prio_down;
    prio_middle = saved_state.prio_middle;
    previous_mask = saved_state.previous_mask;
}

void Translator::operator()(const leaf& l) {
    uint16_t local_prio_up = previous_mask.has_value() ? prio_middle : prio_up;
    uint16_t prio = prio_down / 2 + local_prio_up / 2;
    l.prio_up = local_prio_up;
    l.prio_down = prio_down;
    std::vector<oxm::field_set> sets;
    sets.reserve(l.sets.size());
    for (auto& s: l.sets) {
        if (s.body.has_value()) {
            m_backend.installBarrier(match, prio);
            return;
        }
        sets.push_back(s.pred_actions);
    }
    m_backend.install(match, sets, prio);
}

} // namespace fdd
} // namespace retic
} // namespace runos

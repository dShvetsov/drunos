#pragma once

#include <optional>
#include <memory>

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>

#include <oxm/field.hh>
#include <oxm/field_set.hh>

#include "policies.hh"

namespace runos {
namespace retic {
namespace fdd {

struct action_unit {
    action_unit() = default;
    action_unit(oxm::field_set acts) : pred_actions(acts) { }

    action_unit(oxm::field_set acts, PacketFunction body)
        : pred_actions(acts)
        , body(body)
    { }

    action_unit(oxm::field_set acts, PacketFunction body, const action_unit &post)
        : pred_actions(acts)
        , body(body)
        , post_actions(new action_unit(post))
    { }

    action_unit(oxm::field_set acts, std::optional<PacketFunction> body, const std::unique_ptr<action_unit>& post)
        : pred_actions(acts)
        , body(body)
    {
        if (post != nullptr) {
            post_actions.reset(new action_unit(*post));
         }
    }

    action_unit(const action_unit& acts)
        : pred_actions(acts.pred_actions)
        , body(acts.body)
    {
        if (acts.post_actions != nullptr) {
            post_actions.reset(new action_unit(*acts.post_actions));
        }
    }

    action_unit& operator=(const action_unit& acts) {
        pred_actions = acts.pred_actions;
        body = acts.body;
        if (acts.post_actions != nullptr) {
            post_actions.reset(new action_unit(*acts.post_actions));
        }
        return *this;
    }

    oxm::field_set pred_actions;
    std::optional<PacketFunction> body;
    std::unique_ptr<action_unit> post_actions;

    friend bool operator==(const action_unit& lhs, const action_unit& rhs) {
        bool pred_eq = lhs.pred_actions == rhs.pred_actions;
        bool body_eq = lhs.body == rhs.body;
        if (lhs.post_actions != nullptr) {
            if (rhs.post_actions != nullptr) {
                return pred_eq && body_eq && *lhs.post_actions == *rhs.post_actions;
            } else {
                return false;
            }
        } else if (rhs.post_actions != nullptr) {
            return false;
        } else {
            return pred_eq && body_eq;
        }
        return pred_eq;
    }
};

struct leaf {
    std::vector<action_unit> sets;
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

} // namespace fdd
} // namespace retic
} // namespace runos

#pragma once

#include <memory>

#include "oxm/field_set.hh"

#include "SwitchConnectionFwd.hh"

namespace runos {

struct Actions {
    uint32_t out_port = 0;
    uint32_t group_id = 0;
    oxm::field_set set_fields;
    friend bool operator==(const Actions& lhs, const Actions& rhs) {
        return lhs.out_port == rhs.out_port && lhs.group_id == rhs.group_id && lhs.set_fields == rhs.set_fields;
    }
};

class Rule {
};

class Group {
public:
    virtual uint32_t id() const = 0;
    virtual ~Group() = default;
};

enum class GroupType {
    All
};

using RulePtr = std::shared_ptr<Rule>;
using GroupPtr = std::shared_ptr<Group>;

class OFDriver {
public:
    virtual RulePtr installRule(oxm::field_set match, uint16_t prio, Actions actions, uint8_t table) = 0;
    virtual GroupPtr installGroup(GroupType type, std::vector<Actions> buckets) = 0;
    virtual ~OFDriver() = default;
};

using OFDriverPtr = std::shared_ptr<OFDriver>;

OFDriverPtr makeDriver(SwitchConnectionPtr conn);
} // namespace runos

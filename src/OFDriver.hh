#pragma once

#include <memory>

#include "oxm/field_set.hh"

#include "SwitchConnectionFwd.hh"

namespace runos {

namespace ports {
    static constexpr uint32_t drop = 0;
    static constexpr uint32_t to_controller = 0xfffffffd; // TODO: Unhadrcode
}

struct Actions {
    uint32_t out_port = 0;
    uint32_t group_id = 0;
    uint32_t idle_timeout = 0; // timeouts in seconds
    uint32_t hard_timeout = 0; // zero means infinity timeouts
                               // TODO: Not here
                               // TODO: what is need to do, when flow deleted by timeout
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
    virtual void packetOut(uint8_t* data, size_t data_len, Actions action) = 0;
    virtual ~OFDriver() = default;
};

using OFDriverPtr = std::shared_ptr<OFDriver>;

OFDriverPtr makeDriver(SwitchConnectionPtr conn);
} // namespace runos

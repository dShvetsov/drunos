#pragma once

#include <unordered_map>
#include <variant>
#include <vector>

#include "Application.hh"
#include "Loader.hh"
#include "retic/policies.hh"
#include "retic/backend.hh"
#include "OFDriver.hh"


class Retic: public Application {
SIMPLE_APPLICATION(Retic, "retic")
public:
    void init(Loader* loader, const Config& config) override;

private:
    runos::retic::policy m_policy = runos::retic::fwd(3);
};


namespace runos {
class Of13Backend : public retic::Backend {
public:
    Of13Backend(std::unordered_map<uint64_t, OFDriverPtr> drivers, uint8_t table = 0)
        : m_drivers(drivers), m_table(table)
    { }

    void install(oxm::field_set match, std::vector<oxm::field_set> actions, uint16_t prio) override;
private:
    void install_on(uint64_t dpid, oxm::field_set match, std::vector<oxm::field_set> actions, uint16_t prio);
    std::unordered_map<uint64_t, OFDriverPtr> m_drivers;
    using OfObject = std::variant<GroupPtr, RulePtr>;
    std::vector<OfObject> m_storage;
    uint8_t m_table;
};
} // namespace runos

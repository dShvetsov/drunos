#pragma once

#include <unordered_map>
#include <memory>
#include <variant>
#include <vector>

#include "Application.hh"
#include "Loader.hh"
#include "retic/policies.hh"
#include "retic/backend.hh"
#include "retic/fdd.hh"
#include "OFDriver.hh"
#include "SwitchConnection.hh"
#include <fluid/of13msg.hh>

namespace runos {
    struct Of13Backend;
}

class Retic : public Application
{
SIMPLE_APPLICATION(Retic, "retic")
public:
    void init(Loader* loader, const Config& config) override;
    void registerPolicy(std::string name, runos::retic::policy policy);

    const std::string& getMainName() const { return m_main_policy; }
    runos::retic::policy getMainPolicy() const { return m_policies.at(m_main_policy); }
    std::vector<std::string> getPoliciesName() const;

    void clearRules();
    void reinstallRules();
    void setMain(std::string new_main);

public slots:
    void onSwitchUp(runos::SwitchConnectionPtr conn, fluid_msg::of13::FeaturesReply fr);

private:
    std::unordered_map<std::string, runos::retic::policy> m_policies;
    retic::fdd::diagram m_fdd;
    std::string m_main_policy;

    std::unordered_map<uint64_t, runos::OFDriverPtr> m_drivers;
    std::unique_ptr<runos::Of13Backend> m_backend;
    uint8_t m_table;
};


namespace runos {
class Of13Backend : public retic::Backend {
public:
    Of13Backend(std::unordered_map<uint64_t, OFDriverPtr> drivers, uint8_t table = 0)
        : m_drivers(drivers), m_table(table)
    { }

    void install(oxm::field_set match, std::vector<oxm::field_set> actions, uint16_t prio) override;
    void installBarrier(oxm::field_set match, uint16_t prio) override;
private:
    void install_on(uint64_t dpid, oxm::field_set match, std::vector<oxm::field_set> actions, uint16_t prio);
    std::unordered_map<uint64_t, OFDriverPtr> m_drivers;
    using OfObject = std::variant<GroupPtr, RulePtr>;
    std::vector<OfObject> m_storage;
    uint8_t m_table;
};
} // namespace runos

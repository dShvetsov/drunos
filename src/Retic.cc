#include "Retic.hh"

#include "Controller.hh"
#include "Common.hh"
#include "oxm/openflow_basic.hh"
#include "retic/applier.hh"
#include "retic/fdd.hh"
#include "retic/fdd_compiler.hh"
#include "retic/fdd_translator.hh"
#include "PacketParser.hh"

REGISTER_APPLICATION(Retic, {"controller", ""})

using namespace runos;

void Retic::init(Loader* loader, const Config& root_config)
{
    this->registerPolicy("__builtin_donothing__", retic::stop());
    auto ctrl = Controller::get(loader);
    m_table = ctrl->getTable("retic");
    Config config = config_cd(root_config, "retic");
    m_main_policy = config_get(config, "main", "__builtin_donothing__");
    LOG(INFO) << "Main policy: " << m_main_policy;

    QObject::connect(ctrl, &Controller::switchUp, this, &Retic::onSwitchUp);
}

void Retic::registerPolicy(std::string name, retic::policy policy) {
    LOG(INFO) << "Register policy: " << name;
    m_policies[name] = policy;
}

void Retic::onSwitchUp(SwitchConnectionPtr conn, of13::FeaturesReply fr) {
    m_backend = nullptr;
    m_drivers[conn->dpid()] = makeDriver(conn);
    this->reinstallRules();
}

std::vector<std::string> Retic::getPoliciesName() const {
    std::vector<std::string> ret;
    ret.reserve(m_policies.size());
    for (const auto& [name, pol]: m_policies) {
        ret.push_back(name);
    }
    return ret;
}

void Retic::clearRules() {
    m_backend = nullptr;
}

void Retic::reinstallRules() {
    retic::fdd::diagram d = retic::fdd::compile(m_policies[m_main_policy]);
    m_backend = std::make_unique<Of13Backend>(m_drivers, m_table);
    retic::fdd::Translator translator(*m_backend);
    boost::apply_visitor(translator, d);
}

void Retic::setMain(std::string new_main) {
    m_main_policy = new_main;
    this->reinstallRules();
}

namespace runos {

void Of13Backend::install(oxm::field_set match, std::vector<oxm::field_set> actions, uint16_t prio) {
    static const auto ofb_switch_id = oxm::switch_id();
    auto switch_id_it = match.find(oxm::type(ofb_switch_id));
    if (switch_id_it != match.end()) {
        Packet& pkt_iface(match);
        uint64_t dpid = pkt_iface.load(ofb_switch_id);
        match.erase(oxm::mask<>(ofb_switch_id));
        install_on(dpid, match, actions, prio);
    } else {
        for (auto [dpid, driver]: m_drivers) {
            install_on(dpid, match, actions, prio);
        }
    }
}

void Of13Backend::install_on(uint64_t dpid, oxm::field_set match, std::vector<oxm::field_set> actions, uint16_t prio) {
    static const auto ofb_out_port = oxm::out_port();
    auto driver_it = m_drivers.find(dpid);
    if (driver_it == m_drivers.end()) {
        LOG(WARNING) << "Needed to install rule. But there is no such switch";
        return;
    }

    OFDriverPtr driver = driver_it->second;

    if (actions.empty()) {
        // drop packet
        driver->installRule(match, prio, {}, m_table);
        return;
    } 
    std::vector<Actions> buckets;
    buckets.reserve(actions.size());
    for (auto& action: actions) {
        Actions driver_acts{};
        auto out_port_it = action.find(oxm::type(ofb_out_port));
        if (out_port_it != action.end()) {
            Packet& pkt_iface(action);
            uint32_t out_port = pkt_iface.load(ofb_out_port);
            action.erase(oxm::mask<>(ofb_out_port));
            driver_acts.out_port = out_port;
            driver_acts.set_fields = action;
            buckets.push_back(driver_acts);
        }
    }

    if (buckets.empty()) {
        // install drop rule
        auto flow = driver->installRule(match, prio, {}, m_table);
        m_storage.push_back(flow);
    } else if(buckets.size() == 1) {
        // one actoinlist install directly into flow
        auto flow = driver->installRule(match, prio, buckets[0], m_table);
        m_storage.push_back(flow);
    } else {
        // many actionlists, create Group

        auto group = driver->installGroup(GroupType::All, buckets);
        m_storage.push_back(group);
        Actions to_group = {.group_id = group->id()};
        auto flow = driver->installRule(match, prio, to_group, m_table);
        m_storage.push_back(flow);
    }
}

} // namespace runos

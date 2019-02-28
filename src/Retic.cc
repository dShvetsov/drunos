#include "Retic.hh"

#include "Controller.hh"
#include "Common.hh"
#include "oxm/openflow_basic.hh"
#include "retic/applier.hh"
#include "PacketParser.hh"

REGISTER_APPLICATION(Retic, {"controller", ""})

using namespace runos;

void Retic::init(Loader* loader, const Config& config)
{
    auto ctrl = Controller::get(loader);
    const auto ofb_in_port = oxm::in_port();

    ctrl->registerHandler<of13::PacketIn>(
            [=](of13::PacketIn& pi, SwitchConnectionPtr conn) {
                LOG(INFO) << "Packet in";
                uint8_t buffer[1500];
                PacketParser pp { pi, conn->dpid() };
                retic::Applier<PacketParser> runtime{pp};
                boost::apply_visitor(runtime, m_policy);
                auto& results = runtime.results();
                for (auto& [p, meta]: results) {
                    const Packet& pkt{p};
                    of13::PacketOut po;
                    po.xid(0x1234);
                    po.buffer_id(OFP_NO_BUFFER);
                    uint32_t out_port = pkt.load(oxm::out_port());
                    LOG(WARNING) << "Output to " << out_port << " port";
                    if (out_port != 0) {
                        po.in_port(pkt.load(oxm::in_port()));
                        po.add_action(new of13::OutputAction(out_port, 0));
                        size_t len = p.total_bytes();
                        p.serialize_to(sizeof(buffer), buffer);
                        po.data(buffer, len);
                        conn->send(po);
                    }
                }
            }
        );
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
    OFDriverPtr driver = m_drivers.at(dpid);
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
        Actions to_group = {.group_id = group->id()};
        auto flow = driver->installRule(match, prio, to_group, m_table);
        m_storage.push_back(flow);
    }
}

} // namespace runos

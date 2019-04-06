#include "Application.hh"
#include "Common.hh"
#include "Retic.hh"
#include "retic/policies.hh"
#include "LearningSwitch.hh"
#include "LinkDiscovery.hh"
#include "LLDP.hh"
#include "types/packet_headers.hh"


using namespace runos;
using namespace retic;

class TestApps: public Application {
SIMPLE_APPLICATION(TestApps, "test-apps")
public:
    void init(Loader* loader, const Config& config) override
    {
        auto retic = Retic::get(loader);
        const static auto in_port = oxm::in_port();
        const static auto switch_id = oxm::switch_id();
        retic->registerPolicy("twoports", fwd(1) + fwd(2));
        retic->registerPolicy("twoinports", (filter(in_port == 2) >> fwd(1)) +
                                          (filter(in_port == 1) >> fwd(2)));
        retic->registerPolicy("dropall", stop());
        retic->registerPolicy("two_switch",
            filter(switch_id == 1) >> (fwd(1) + fwd(2)) | filter(switch_id == 2) >> (fwd(1) + fwd(2))
        );
        retic->registerPolicy("func", handler([](Packet& pkt){ return fwd(1) + fwd(2); }));

        // TODO: make it withou dynamic_cast
        auto link_discovery = dynamic_cast<LinkDiscovery*> (LinkDiscovery::get(loader));

        if (link_discovery == nullptr) {
            LOG(ERROR) << "Link discoery accidently is nullptr";
        }

        retic->registerPolicy("with_link_discovery",
            link_discovery->getPolicy() +
            (
                filter_not(oxm::eth_type() == LLDP_ETH_TYPE) >> (fwd(1) + fwd(2) + fwd(3))
            )
        );
        auto learning_switch = LearningSwitch::get(loader);
        policy ipv6_dropper = not(
            filter(oxm::eth_type() == 0x86dd)
        );

        policy not_lldp = filter_not(oxm::eth_type() == LLDP_ETH_TYPE);

        policy lswitch = filter_not(oxm::eth_type() == LLDP_ETH_TYPE) >>
                         learning_switch->getPolicy();

        retic->registerPolicy("learning-switch",
            link_discovery->getPolicy() | ipv6_dropper >> lswitch
        );

        retic->registerPolicy("test-policy",
            link_discovery->getPolicy() |
            not_lldp >> firewall() >> learning_switch->getPolicy());
    }

    policy basic_firewall() {
        static constexpr auto eth_src = oxm::eth_src();
        static constexpr auto eth_type = oxm::eth_type();
        constexpr uint16_t ipv6 = 0x86dd;
        return filter_not(eth_src == "ff:ff:ff:ff:ff:ff") >>
               filter_not(eth_type == ipv6);
    }

    policy bad_ip_reciever() {
        static constexpr auto eth_type = oxm::eth_type();
        static constexpr auto ip_dst = oxm::ipv4_dst();
        constexpr uint16_t ipv4 = 0x0800;
        return if_then_else(eth_type == ipv4,
                //then
                    filter_not(ip_dst == "10.0.0.2") >>
                    filter_not(ip_dst == "10.0.0.3"),
                //else
                    id()
                );
    }

    policy bad_tcp() {
        static constexpr auto eth_type = oxm::eth_type();
        static constexpr auto port = oxm::udp_dst();
        static constexpr auto ip_proto = oxm::ip_proto();
        constexpr uint16_t ipv4 = 0x0800;
        // constexpr uint8_t tcp = 0x06;
        constexpr uint8_t udp = 0x11;
        return if_then_else(eth_type == ipv4,
                // then
                   if_then_else(ip_proto == udp,
                    // then
                        filter_not(port == 2220) >>
                        filter_not(port == 2221) >>
                        filter_not(port == 2222) >>
                        filter_not(port == 2223) >>
                        filter_not(port == 2224),
                    // else
                        id()
                    ),
                //else
                   id()
                );
    }

    policy firewall() {
        // return basic_firewall() >> bad_ip_reciever() >> bad_tcp();
        return basic_firewall() >> bad_ip_reciever() >> bad_tcp();
    }
};

REGISTER_APPLICATION(TestApps, {"retic", "link-discovery", "learning-switch", ""})


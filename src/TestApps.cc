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

        policy lswitch = filter_not(oxm::eth_type() == LLDP_ETH_TYPE) >>
                         learning_switch->getPolicy();

        retic->registerPolicy("learning-switch",
            link_discovery->getPolicy() | ipv6_dropper >> lswitch
        );
    }
};

REGISTER_APPLICATION(TestApps, {"retic", "link-discovery", "learning-switch", ""})


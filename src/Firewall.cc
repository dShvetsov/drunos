/*
 *
 * Application for testing runos 0.7 vs drunos 0.8
 * Just put it into https://github.com/ARCCN/runos/releases/tag/v0.7 : runos/src
 * Add Firewall.cc into CMakeLists
 * And use network-settings from thisbrach:functest/network-settings-maple.json
 *
 */
#include <algorithm>

#include "Application.hh"
#include "Loader.hh"
#include "Maple.hh"
#include "Decision.hh"
#include "api/Packet.hh"

#include "oxm/openflow_basic.hh"

template<class Iterable>
bool match_any(const Iterable& container, const Packet& p){
    return std::any_of(container.begin(), container.end(),
        [&p](oxm::field<> f) {
            return p.test(f);
        }
   );
}

class Firewall : public Application {
SIMPLE_APPLICATION(Firewall, "firewall")
public:
    void init(Loader* loader, const Config& config) override
    {
        auto maple = Maple::get(loader);
        static constexpr auto ip_dst = oxm::ipv4_dst();
        static constexpr auto port = oxm::udp_dst();
        static constexpr auto eth_type = oxm::eth_type();
        static constexpr auto eth_src = oxm::eth_src();
        static const auto broadcast = ethaddr("ff:ff:ff:ff:ff:ff");
        static constexpr auto ip_proto = oxm::ip_proto();
        static constexpr uint16_t ipv4 = 0x0800;
        static constexpr uint8_t udp = 0x11;
        static constexpr uint16_t ipv6 = 0x86dd;



        static const std::vector<oxm::field<>> bad_ip = {
            ip_dst == "10.0.0.2",
            ip_dst == "10.0.0.3'"
        };
        static const std::vector<oxm::field<>> bad_udp = {
            port == 2220,
            port == 2221,
            port == 2222,
            port == 2223,
            port == 2224
        };

        maple->registerHandler("drop-all",
            [=](Packet& pkt, FlowPtr, Decision decision) {
                if (pkt.test(eth_src == broadcast) ||
                    pkt.test(eth_type == ipv6)) {
                    return decision.drop().return_();
                }
                if (pkt.test(eth_type == ipv4)) {
                    if (match_any(bad_ip, pkt)) {
                        return decision.drop().return_();
                    }
                    if (pkt.test(ip_proto == udp)) {
                        if (match_any(bad_udp, pkt)) {
                            return decision.drop().return_();
                        }
                    }
                }
                return decision;
            }
        );
    }
};

REGISTER_APPLICATION(Firewall, {"maple", ""})

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/applier.hh"
#include "retic/policies.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"
#include "types/packet_headers.hh"
#include "PacketParser.hh"
#include "fluid/of13msg.hh"

#include <boost/lexical_cast.hpp>

#include "openflow/openflow-1.3.5.h"

using namespace runos;
using namespace retic;

TEST(PacketParserTest, OutPortAfterPolicy)
{
    policy p = fwd(3);
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    PacketParser pkt(pi, 1);
    Applier<PacketParser> applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    //ASSERT_THAT(results, UnorderedElementsAre(
    //            Key(ResultOf([](auto& pkt) {
    //                    return pkt.get_out_port();
    //                }, 3))
    //    ));
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](auto& p) {
                        const Packet& pkt{p};
                        return int(pkt.load(oxm::out_port()));
                    }, 3))
        ));
}

TEST(DISABLED_PacketParserTest, ApplyTwoActions)
{
    policy p = handler([](Packet& pkt) {return fwd(1) + fwd(2); });

    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    PacketParser pkt(pi, 1);
    Applier<PacketParser> applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
        Key(ResultOf([](auto& p){ const Packet& pkt{p}; return int(pkt.load(oxm::out_port()));}, 1)),
        Key(ResultOf([](auto& p){ const Packet& pkt{p}; return int(pkt.load(oxm::out_port()));}, 2))
    ));
}

TEST(PacketParserTest, ModifyOutPort)
{
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    PacketParser pkt(pi, 1);
    pkt.modify(oxm::out_port() << 3);

    // EXPECT_EQ(3, pkt.get_out_port());
}

TEST(PacketParserTest, ModifyValue)
{
    ethernet_hdr eth;
    eth.dst = 0x123;
    eth.src = 0x321;
    eth.type = 0;
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    pi.data(&eth, eth.header_length());

    PacketParser pp(pi, 1);
    Packet& pkt{pp};
    pkt.modify(oxm::eth_src() << "11:22:33:44:55:66");

    constexpr auto ofb_eth_src = oxm::eth_src();
    ethaddr addr = pkt.load(ofb_eth_src);
    EXPECT_EQ("11:22:33:44:55:66", boost::lexical_cast<std::string>(addr));
}

TEST(PacketParserTest, ReticModifyValue)
{
    ethernet_hdr eth;
    eth.dst = 0x123;
    eth.src = 0x321;
    eth.type = 0;
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    pi.data(&eth, eth.header_length());

    PacketParser pp(pi, 1);

    policy p = modify(oxm::eth_src() << "11:22:33:44:55:66");

    Applier<PacketParser> applier{pp};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](auto& p) {
                        const Packet& pkt{p};
                        ethaddr addr = pkt.load(oxm::eth_src());
                        return boost::lexical_cast<std::string>(addr);
                    }, "11:22:33:44:55:66"))
        ));
}

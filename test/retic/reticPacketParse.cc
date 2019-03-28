#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.hh"

#include "retic/applier.hh"
#include "retic/policies.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"
#include "types/packet_headers.hh"
#include "PacketParser.hh"
#include "fluid/of13msg.hh"
#include "api/Packet.hh"

#include <memory>
#include <boost/lexical_cast.hpp>

#include "openflow/openflow-1.3.5.h"

using namespace runos;
using namespace retic;

using PacketPtr = std::shared_ptr<Packet>;

TEST(PacketParserTest, OutPortAfterPolicy)
{
    policy p = fwd(3);
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    PacketParser pkt(pi, 1);
    Applier applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    //ASSERT_THAT(results, UnorderedElementsAre(
    //            Key(ResultOf([](auto& pkt) {
    //                    return pkt.get_out_port();
    //                }, 3))
    //    ));
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](PacketPtr p) {
                        return int(p->load(oxm::out_port()));
                    }, 3))
        ));
}

TEST(PacketParserTest, ApplyTwoActions)
{
    policy p = handler([](Packet& pkt) {return fwd(1) + fwd(2); });

    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    PacketParser pkt(pi, 1);
    Applier applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
        Key(ResultOf([](PacketPtr p){ return int(p->load(oxm::out_port()));}, 1)),
        Key(ResultOf([](PacketPtr p){ return int(p->load(oxm::out_port()));}, 2))
    ));
}

TEST(PacketParserTest, Clone)
{
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    struct eth_hdr {
        uint8_t dst[6];
        uint8_t src[6];
        uint16_t type;
    };

    eth_hdr data{.dst={0x11, 0x22, 0x33, 0x44, 0x55, 0x66}, .src={0xaa,0xbb,0xcc,0xdd,0xee,0xff}, .type=0};

    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    pi.data(reinterpret_cast<uint8_t*> (&data), sizeof(data));

    PacketParser pkt(pi, 1, 21);
    auto pkt2 = pkt.clone();
    pkt.modify(oxm::out_port() << 2);
    Packet& pkt_iface(*pkt2);

    ASSERT_EQ(21, pkt_iface.load(oxm::out_port()));
    ASSERT_TRUE(pkt_iface.test(oxm::eth_dst() == "11:22:33:44:55:66"));
    ASSERT_TRUE(pkt_iface.test(oxm::eth_src() == "aa:bb:cc:dd:ee:ff"));

}

TEST(FieldSetTest, Clone) 
{
    oxm::field_set fs{F<1>() == 1, F<2>() == 2, F<3>() == 3};
    auto fs2 = fs.clone();
    fs.modify(F<1>() == 10);
    fs2->modify(F<2>() == 20);
    oxm::field_set fs_true_value{F<1>() == 10, F<2>() == 2, F<3>() == 3};
    ASSERT_EQ(fs_true_value, fs);
    ASSERT_EQ(1, fs2->load(F<1>()));
    ASSERT_EQ(20, fs2->load(F<2>()));
    ASSERT_EQ(3, fs2->load(F<3>()));

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

    Applier applier{pp};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](PacketPtr p) {
                        ethaddr addr = p->load(oxm::eth_src());
                        return boost::lexical_cast<std::string>(addr);
                    }, "11:22:33:44:55:66"))
        ));
}

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/policies.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"
#include "PacketParser.hh"
#include "fluid/of13msg.hh"

#include "openflow/openflow-1.3.5.h"

using namespace runos;
using namespace retic;

TEST(ReticTest, PacketParser)
{
    policy p = fwd(3);
    fluid_msg::of13::PacketIn pi(10, OFP_NO_BUFFER, 0, 0, 0, 0);
    pi.add_oxm_field(new fluid_msg::of13::InPort(2));
    PacketParser pkt(pi, 1);
    Applier<PacketParser> applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();

    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](auto& pkt) {
                        return pkt.get_out_port();
                    }, 3))
        ));

}

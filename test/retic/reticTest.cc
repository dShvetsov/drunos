#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/policies.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"

#include "openflow/openflow-1.3.5.h"

#include <boost/variant/apply_visitor.hpp>

using namespace runos;

template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };

TEST(FilterTest, TrueFilter)
{
    policy p = filter(F<1>() == 100);
    oxm::field_set packet = {
        { F<1>() == 100 }
    };
    Applier<oxm::field_set> applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt, result] : results )
    {
        EXPECT_FALSE(result.stopped) <<
            "Filter mustn't stop packet with same field";
    }
}

TEST(FilterTest, FalseFilter)
{
    policy p = filter(F<1>() == 100);
    oxm::field_set packet = {
        { F<1>() == 200 }
    };
    Applier<oxm::field_set> applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt, result] : results )
    {
        EXPECT_TRUE(result.stopped) <<
            "Filter must stop packet with wrong field";
    }
}

TEST(StopTest, StopTest)
{
    policy p = stop();
    oxm::field_set packet;
    Applier<oxm::field_set> applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt, result] : results )
    {
        EXPECT_TRUE(result.stopped) <<
            "Filter must stop packet with wrong field";
    }

}

TEST(ForwardTest, ForwardToPort)
{
    policy p = fwd(1);
    oxm::field_set packet;
    Applier<oxm::field_set> applier{packet};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();
    for (auto& [pkt, result] : results)
    {
        EXPECT_EQ(1, result.port);
    }
}

TEST(ModifyTest, ModifyField)
{
    policy p = modify(F<1>() << 100);
    oxm::field_set packet = {
        { F<1>() == 200 },
        { F<2>() == 300 }
    };
    Applier<oxm::field_set> applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt,results] : results) {
        EXPECT_TRUE(pkt.test(F<1>() == 100)) <<
            "Modify must change the field";

        EXPECT_TRUE(pkt.test(F<2>() == 300)) <<
            "Modify must not change other field";
    }
}

TEST(SeqTest, SecondPolicy)
{
    policy seq_policy = Sequential{fwd(1), fwd(2)};

    oxm::field_set packet;
    Applier<oxm::field_set> applier{packet};

    boost::apply_visitor(applier, seq_policy);
    auto& results = applier.results();
    for (auto& [pkt, result] : results) {
        EXPECT_EQ(2, result.port) << "Second policy must be applied";
    }
}

TEST(SeqTest, AllPolicy)
{
    policy seq_policy = Sequential{modify(F<1>() << 100), modify(F<2>() << 100)};

    oxm::field_set packet {
        {F<1>() == 0},
        {F<2>() == 0}
    };
    Applier<oxm::field_set> applier{packet};

    boost::apply_visitor(applier, seq_policy);
    auto& results = applier.results();
    for (auto& [pkt, result] : results) {
        EXPECT_TRUE(pkt.test(F<1>() == 100)) << "All policy must be applied";
        EXPECT_TRUE(pkt.test(F<2>() == 100)) << "All policy must be applied";
    }
}


TEST(SeqTest, StopPolicy)
{
    policy seq_policy = Sequential{stop(), fwd(2)};

    oxm::field_set packet;
    Applier<oxm::field_set> applier{packet};

    boost::apply_visitor(applier, seq_policy);
    auto& results = applier.results();
    for (auto& [pkt, result] : results) {
        EXPECT_EQ(0, result.port) <<
            "Second policy must not be applied if pipeline is stopped";
    }
}

TEST(ParallelTest, ParallelPolicy)
{
    policy parallel = Parallel{modify(F<1>() << 100), modify(F<1>() << 200)};

    oxm::field_set packet = {
        {F<1>() == 0}
    };

    Applier<oxm::field_set> applier{packet};

    boost::apply_visitor(applier, parallel);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, SizeIs(2));
    EXPECT_THAT(results, UnorderedElementsAre(
                    Key(ResultOf([](auto& pkt){return pkt.test(F<1>() == 100);}, true)),
                    Key(ResultOf([](auto& pkt){return pkt.test(F<1>() == 200);}, true))
                ));
}

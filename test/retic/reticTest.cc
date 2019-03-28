#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "common.hh"

#include "retic/policies.hh"
#include "retic/applier.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"

#include "openflow/openflow-1.3.5.h"

#include <boost/variant/apply_visitor.hpp>

using namespace runos;
using namespace retic;

using PacketPtr = std::shared_ptr<Packet>;

TEST(FilterTest, TrueFilter)
{
    policy p = filter(F<1>() == 100);
    oxm::field_set packet = {
        { F<1>() == 100 }
    };
    Applier applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt, result] : results )
    {
        EXPECT_FALSE(result.isStopped()) <<
            "Filter mustn't stop packet with same field";
    }
}

TEST(FilterTest, FalseFilter)
{
    policy p = filter(F<1>() == 100);
    oxm::field_set packet = {
        { F<1>() == 200 }
    };
    Applier applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt, result] : results )
    {
        EXPECT_TRUE(result.isStopped()) <<
            "Filter must stop packet with wrong field";
    }
}

TEST(StopTest, StopTest)
{
    policy p = stop();
    oxm::field_set packet;
    Applier applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt, result] : results )
    {
        EXPECT_TRUE(result.isStopped()) <<
            "Filter must stop packet with wrong field";
    }

}

TEST(ForwardTest, ForwardToPort)
{
    policy p = fwd(1);
    oxm::field_set packet;
    Applier applier{packet};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();
    for (auto& [pkt, result] : results)
    {
        EXPECT_TRUE(pkt->test(oxm::out_port() == 1));
    }
}

TEST(ModifyTest, ModifyField)
{
    policy p = modify(F<1>() << 100);
    oxm::field_set packet = {
        { F<1>() == 200 },
        { F<2>() == 300 }
    };
    Applier applier{packet};
    boost::apply_visitor(applier, p);

    auto& results = applier.results();
    for (auto& [pkt,results] : results) {
        EXPECT_TRUE(pkt->test(F<1>() == 100)) <<
            "Modify must change the field";

        EXPECT_TRUE(pkt->test(F<2>() == 300)) <<
            "Modify must not change other field";
    }
}

TEST(SeqTest, SecondPolicy)
{
    policy seq_policy = fwd(1) >> fwd(2);

    oxm::field_set packet;
    Applier applier{packet};

    boost::apply_visitor(applier, seq_policy);
    auto& results = applier.results();
    for (auto& [pkt, result] : results) {
        EXPECT_TRUE(pkt->test(oxm::out_port() == 2)) << "Second policy must be applied";
    }
}

TEST(SeqTest, AllPolicy)
{
    policy seq_policy = modify(F<1>() << 100) >> modify(F<2>() << 100);

    oxm::field_set packet {
        {F<1>() == 0},
        {F<2>() == 0}
    };
    Applier applier{packet};

    boost::apply_visitor(applier, seq_policy);
    auto& results = applier.results();
    for (auto& [pkt, result] : results) {
        EXPECT_TRUE(pkt->test(F<1>() == 100)) << "All policy must be applied";
        EXPECT_TRUE(pkt->test(F<2>() == 100)) << "All policy must be applied";
    }
}


TEST(SeqTest, StopPolicy)
{
    policy seq_policy = stop() >> fwd(2);

    oxm::field_set packet;
    Applier applier{packet};

    boost::apply_visitor(applier, seq_policy);
    auto& results = applier.results();
    for (auto& [pkt, result] : results) {
        EXPECT_TRUE(pkt->test(oxm::out_port() == 2)) <<
            "Second policy must not be applied if pipeline is stopped";
    }
}

TEST(ParallelTest, ParallelPolicy)
{
    policy parallel = modify(F<1>() << 100) + modify(F<1>() << 200);

    oxm::field_set packet = {
        {F<1>() == 0}
    };

    Applier applier{packet};

    boost::apply_visitor(applier, parallel);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, SizeIs(2));
    EXPECT_THAT(results, UnorderedElementsAre(
                    Key(ResultOf([](PacketPtr pkt){return pkt->test(F<1>() == 100);}, true)),
                    Key(ResultOf([](PacketPtr pkt){return pkt->test(F<1>() == 200);}, true))
                ));
}

TEST(ComplexTest, Test1)
{
    policy f = filter(F<1>() == 100) + filter(F<2>() == 200);
    policy modify_and_forward = (modify(F<1>() << 300) >> fwd(666)) + fwd(123);
    policy all = f >> modify_and_forward;

    oxm::field_set packet = {
        {F<1>() == 100},
        {F<2>() == 200}
    };

    Applier applier{packet};
    boost::apply_visitor(applier, all);
    auto& results = applier.results();
    using namespace ::testing;

    EXPECT_THAT(results, Contains(
                Key(ResultOf([](PacketPtr pkt){
                        return pkt->test(F<1>() == 300) &&
                               pkt->test(F<2>() == 200) &&
                               pkt->test(oxm::out_port() == 666);
                        }, true))
        ));
    EXPECT_THAT(results, Contains(
                Key(ResultOf([](PacketPtr pkt){
                        return pkt->test(F<1>() == 100) &&
                               pkt->test(F<2>() == 200) &&
                               pkt->test(oxm::out_port() == 123);
                        }, true))
        ));

}

TEST(FunctionTest, SimpleFunction)
{
    auto f = [](Packet& pkt) {
        return modify(F<1>() << 100);
    };

    policy h = handler(f);
    oxm::field_set packet = {
        {F<1>() == 1}
    };

    Applier applier{packet};
    boost::apply_visitor(applier, h);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](PacketPtr pkt) {
                        return pkt->test(F<1>() == 100);
                    }, true))
        ));
}

TEST(FunctionTest, FunctionWithFilter1)
{
    auto f = [](Packet& pkt) {
        if (pkt.test(F<1>() == 100)) {
            return fwd(123);
        } else {
            return fwd(321);
        }
    };

    policy h = handler(f);

    oxm::field_set packet = {
        {F<1>() == 100}
    };

    Applier applier{packet};
    boost::apply_visitor(applier, h);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](PacketPtr pkt) {
                        return pkt->test(oxm::out_port() == 123);
                    }, true))
        ));
}

TEST(FunctionTest, FunctionWithFilter2)
{
    auto f = [](Packet& pkt) {
        if (pkt.test(F<1>() == 100)) {
            return fwd(123);
        } else {
            return fwd(321);
        }
    };

    policy h = handler(f);

    oxm::field_set packet = {
        {F<1>() == 200}
    };

    Applier applier{packet};
    boost::apply_visitor(applier, h);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
                Key(ResultOf([](PacketPtr pkt) {
                        return pkt->test(oxm::out_port() == 321);
                    }, true))
        ));
}

TEST(FunctionTest, OutputToTwoPorts) 
{
    policy p = handler([](Packet& pkt) {return fwd(1) + fwd(2); });

    oxm::field_set pkt {oxm::out_port() == 0};
    Applier applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
        Key(ResultOf([](PacketPtr p) {return p->load(oxm::out_port());}, 1)),
        Key(ResultOf([](PacketPtr p) {return p->load(oxm::out_port());}, 2))
    ));
}

TEST(PolicyTest, IdTest) {
    policy p = id();
    oxm::field_set pkt {F<1>() == 1};
    Applier applier{pkt};
    boost::apply_visitor(applier, p);
    auto& results = applier.results();
    using namespace ::testing;
    ASSERT_THAT(results, UnorderedElementsAre(
        Key(ResultOf([](PacketPtr p) {return p->load(F<1>());}, 1))
    ));
}

TEST(DISABLED_EqualPolicyTest, ThreeSeq) {
    policy p1 = modify(F<1>() == 1) >> modify(F<2>() == 2) >> id();
    policy p2 = modify(F<2>() == 2) >> id();
    p2 = modify(F<1>() == 1) >> p2;
    EXPECT_EQ(p1, p2);
}


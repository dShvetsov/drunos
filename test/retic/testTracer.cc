#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.hh"

#include "retic/tracer.hh"
#include "retic/fdd.hh"
#include "retic/policies.hh"
#include "retic/leaf_applier.hh"
#include "oxm/openflow_basic.hh"

using namespace runos;
using namespace retic;
using namespace tracer;
using namespace ::testing;

TEST(TracerTest, ReturnPolicy) {
    policy p = handler([](Packet& pkt){return stop(); });
    oxm::field_set fs;
    Tracer tracer{p};
    auto trace = tracer.trace(fs);
    auto p_result = trace.result();
    boost::get<Stop>(p_result);
}

TEST(DISABLED_TracerTest, FailureOnNonEmptyFieldSet) {
    policy p = handler([](Packet& pkt){return stop();});
    Tracer tracer{p};
    oxm::field_set fs{F<1>() == 1};
    auto trace = tracer.trace(fs);
}

TEST(TracerTest, LoadMethod) {
    oxm::field_set fs{F<1>() == 1};
    policy p = handler([](Packet& pkt){
        pkt.load(F<1>());
        return stop();
    });
    Tracer tracer{p};
    auto trace = tracer.trace(fs);
    ASSERT_THAT(trace.values(), ElementsAre(load_node{F<1>() == 1}));
}

TEST(TracerTest, TestMethod) {
    oxm::field_set fs{F<1>() == 1};
    policy p = handler([](Packet& pkt){
        pkt.test(F<1>() == 1);
        return stop();
    });
    Tracer tracer{p};
    auto trace = tracer.trace(fs);
    ASSERT_THAT(trace.values(), ElementsAre(test_node{F<1>() == 1, true}));
}

TEST(TracerTest, TestModify) {
    oxm::field_set fs{F<1>() == 1};
    policy p = handler([](Packet& pkt){
        pkt.modify(F<1>() == 1);
        pkt.modify(F<2>() == 2);
        return id();
    });
    Tracer tracer{p};
    auto trace = tracer.trace(fs);
    ASSERT_THAT(trace.result(), modify(F<1>() == 1) >> (modify(F<2>() == 2) >> id()));
}

TEST(TracerTest, MergeTraceLoad) {
    Trace trace1, trace2;
    trace1.load(F<1>() == 1);
    trace2.load(F<2>() == 2);
    auto merged_trace = mergeTrace({trace1, trace2});
    EXPECT_THAT(
        merged_trace.values(),
        ElementsAre(load_node{F<1>() == 1}, load_node{F<2>() == 2})
    );
}

TEST(TracerTest, MergeTraceTest) {
    Trace trace1, trace2;
    trace1.test(F<1>() == 1, true);
    trace2.test(F<2>() == 2, false);
    auto merged_trace = mergeTrace({trace1, trace2});
    EXPECT_THAT(
        merged_trace.values(),
        ElementsAre(test_node{F<1>() == 1, true}, test_node{F<2>() == 2, false})
    );
}

TEST(TracerTest, MergeTraceTestCaching) {
    Trace trace1, trace2, trace3;
    trace1.test(F<1>() == 1, true);
    trace2.test(F<1>() == 1, true);
    trace3.load(F<1>() == 1);
    auto merged_trace = mergeTrace({trace1, trace2, trace3});
    EXPECT_THAT(
        merged_trace.values(),
        ElementsAre(test_node{F<1>() == 1, true})
    );
}

TEST(TracerTest, MergeTraceLoadCaching) {
    Trace trace1, trace2;
    trace1.load(F<1>() == 1);
    trace2.load(F<1>() == 1);
    auto merged_trace = mergeTrace({trace1, trace2});
    EXPECT_THAT(
        merged_trace.values(),
        ElementsAre(load_node{F<1>() == 1})
    );
}

TEST(TracerTest, TestResults) {
    Trace trace1, trace2;
    trace1.load(F<1>() == 1);
    trace1.setResult(modify(F<3>() << 3));
    trace2.load(F<2>() == 2);
    trace2.setResult(modify(F<4>() << 4));
    auto merged_trace = mergeTrace({trace1, trace2});
    EXPECT_EQ(merged_trace.result(), modify(F<3>() << 3) + modify(F<4>() << 4));
}

TEST(TestLeafApplier, GetTracerOneHandler) {
    auto p1 = handler([](Packet& pkt) {
        pkt.load(F<1>());
        return id();
    });
    PacketFunction pf1 = boost::get<PacketFunction>(p1);

    fdd::leaf l = {{
        {{oxm::field_set{}}, pf1}
    }};
    oxm::field_set fs1{F<1>() == 1, F<2>() == 2, F<3>() == 3};
    auto traces = getTraces(l, fs1);
    EXPECT_THAT(traces, UnorderedElementsAre(
        AllOf(
            Property(&Trace::values, ElementsAre(load_node{F<1>() == 1})),
            Property(&Trace::result, id())
        )
    ));
}

TEST(TestLeafApplier, GetTracer) {
    auto p1 = handler([](Packet& pkt) {
        pkt.load(F<1>());
        return id();
    });
    auto p2 = handler([](Packet& pkt) {
        pkt.load(F<2>());
        return modify(F<4>() << 4);
    });
    PacketFunction pf1 = boost::get<PacketFunction>(p1);
    PacketFunction pf2 = boost::get<PacketFunction>(p2);

    fdd::leaf l = {{
        {{oxm::field_set{}}, pf1},
        {{oxm::field_set{F<2>() == 2}}, pf2},
        {{oxm::field_set{F<3>() == 3}}}
    }};
    oxm::field_set fs1{F<1>() == 1, F<2>() == 2, F<3>() == 3};
    auto traces = getTraces(l, fs1);
    EXPECT_THAT(traces, UnorderedElementsAre(
        AllOf(
            Property(&Trace::values, ElementsAre(load_node{F<1>() == 1})),
            Property(&Trace::result, id())
        ),
        AllOf(
            Property(&Trace::values, ElementsAre()),
            Property(&Trace::result, modify(F<2>() == 2) >> modify(F<4>() == 4))
        ),
        AllOf(
            Property(&Trace::values, ElementsAre()),
            Property(&Trace::result, modify(F<3>() == 3) >> id())
        )
    ));
}

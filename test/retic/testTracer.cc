#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/tracer.hh"
#include "retic/fdd.hh"
#include "retic/policies.hh"
#include "oxm/openflow_basic.hh"

using namespace runos;
using namespace retic;
using namespace tracer;
using namespace ::testing;

template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };

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

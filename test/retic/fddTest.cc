#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/fdd.hh"
#include "retic/policies.hh"

using namespace runos;
using namespace retic;
using namespace ::testing;


template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };


TEST(FddTest, StopCompile) {
    policy p = stop();
    fdd::diagram diagram = fdd::compile(p);
    fdd::leaf leaf = boost::get<fdd::leaf>(diagram);
    ASSERT_TRUE(leaf.sets.empty());
}

TEST(FddTest, Modify) {
    policy p = modify(F<1>() << 100);
    fdd::diagram diagram = fdd::compile(p);
    fdd::leaf leaf = boost::get<fdd::leaf>(diagram);
    ASSERT_THAT(leaf.sets, SizeIs(1));
    ASSERT_EQ(oxm::field_set{F<1>() == 100}, leaf.sets[0]);
}

TEST(FddTest, Filter) {
    policy p = filter(F<1>() == 100);
    fdd::diagram diagram = fdd::compile(p);
    fdd::node node = boost::get<fdd::node>(diagram);
    oxm::field<> true_value = F<1>() == 100;
    EXPECT_EQ(node.field, true_value);
    fdd::leaf pos = boost::get<fdd::leaf>(node.positive);
    ASSERT_THAT(pos.sets, SizeIs(1));
    EXPECT_TRUE(pos.sets[0].empty());
    fdd::leaf neg = boost::get<fdd::leaf>(node.negative);
    EXPECT_THAT(neg.sets, IsEmpty());

}

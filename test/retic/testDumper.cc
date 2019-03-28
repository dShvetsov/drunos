#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.hh"

#include <iostream>

#include "retic/policies.hh"
#include "retic/dumper.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"

#include "openflow/openflow-1.3.5.h"

using namespace runos;
using namespace retic;

TEST(DumperTest, Filter)
{
    policy p = filter(F<1>() == 100);
    std::stringstream ss;
    Dumper dumper(ss);
    boost::apply_visitor(dumper, p);
    EXPECT_THAT("filter( F<1ul>{} == 00000000000000000000000001100100 )", ss.str());
}

TEST(DumperTest, Modify)
{
    policy p = modify(F<1>() << 100);
    std::stringstream ss;
    Dumper dumper(ss);
    boost::apply_visitor(dumper, p);
    EXPECT_THAT("modify( F<1ul>{} == 00000000000000000000000001100100 )", ss.str());
}

TEST(DumperTest, Stop)
{
    policy p = stop();
    std::stringstream ss;
    Dumper dumper(ss);
    boost::apply_visitor(dumper, p);
    EXPECT_THAT("stop", ss.str());
}

TEST(DumperTest, Seq)
{
    policy p = filter(F<1>() == 100) >> modify(F<2>() == 200);
    std::stringstream ss;
    Dumper dumper(ss);
    boost::apply_visitor(dumper, p);
    EXPECT_THAT("filter( F<1ul>{} == 00000000000000000000000001100100 ) >> modify( F<2ul>{} == 00000000000000000000000011001000 )", ss.str());
}

TEST(DumperTest, Par)
{
    policy p = filter(F<1>() == 100) + filter(F<2>() == 200);
    std::stringstream ss;
    Dumper dumper(ss);
    boost::apply_visitor(dumper, p);
    EXPECT_THAT("filter( F<1ul>{} == 00000000000000000000000001100100 ) + filter( F<2ul>{} == 00000000000000000000000011001000 )", ss.str());
}

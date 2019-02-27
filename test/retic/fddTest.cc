#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/fdd.hh"
#include "retic/policies.hh"
#include "oxm/openflow_basic.hh"

using namespace runos;
using namespace retic;
using namespace ::testing;


template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };


TEST(TypeComprassion, Types) {
    const auto ofb_switch_id = oxm::switch_id();
    const auto ofb_eth_src = oxm::eth_src();
    const auto ofb_eth_type = oxm::eth_type();
    EXPECT_EQ(0, fdd::compare_types(ofb_switch_id, ofb_switch_id));
    EXPECT_EQ(0, fdd::compare_types(ofb_eth_src, ofb_eth_src));
    EXPECT_EQ(0, fdd::compare_types(ofb_eth_type, ofb_eth_type));
    EXPECT_GT(0, fdd::compare_types(ofb_eth_type, ofb_switch_id));
    EXPECT_GT(0, fdd::compare_types(ofb_eth_type, ofb_eth_src));
}

TEST(EqualFdd, EqualTest) {
    fdd::diagram node1 = fdd::node{F<1>() == 1, fdd::leaf{}, fdd::leaf{}};
    fdd::diagram node2 = fdd::node{F<1>() == 1, fdd::leaf{}, fdd::leaf{}};
    fdd::diagram node3 = fdd::node{F<1>() == 1, fdd::leaf{}, fdd::leaf{{oxm::field_set{F<1>() == 1}}}};
    fdd::diagram node4 = fdd::node{F<1>() == 2, fdd::leaf{}, fdd::leaf{}};
    fdd::diagram node5 = fdd::node{F<2>() == 1, fdd::leaf{}, fdd::leaf{}};
    fdd::diagram node6 = fdd::node{F<1>() == 1, fdd::leaf{{oxm::field_set{F<1>() == 1}}}, fdd::leaf{}};

    fdd::diagram node7 = fdd::node{F<1>() == 1, fdd::leaf{}, fdd::leaf{{oxm::field_set{F<2>() == 2}, oxm::field_set{F<1>() == 1}}}};
    fdd::diagram node8 = fdd::node{F<1>() == 1, fdd::leaf{}, fdd::leaf{{oxm::field_set{F<1>() == 1}, oxm::field_set{F<2>() == 2}}}};

    EXPECT_EQ(node1, node2);
    EXPECT_NE(node1, node3);
    EXPECT_NE(node1, node4);
    EXPECT_NE(node1, node5);
    EXPECT_NE(node1, node5);
    EXPECT_EQ(node7, node8);
}


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

TEST(FddTest, ParallelLeafLeaf) {
    policy p = modify(F<1>() << 100) + modify(F<2>() << 200);
    fdd::diagram diagram = fdd::compile(p);
    fdd::leaf leaf = boost::get<fdd::leaf>(diagram);
    ASSERT_THAT(leaf.sets, SizeIs(2));
    EXPECT_THAT(leaf.sets,
        UnorderedElementsAre(
            oxm::field_set{F<1>() == 100},
            oxm::field_set{F<2>() == 200}
        ));
}

TEST(FddTest, ParallellNodeLeaf) {
    policy p = filter(F<1>() == 1) + modify(F<3>() << 3);
    fdd::diagram diagram = fdd::compile(p);
    fdd::node node = boost::get<fdd::node>(diagram);
    oxm::field<> true_value = F<1>() == 1;
    EXPECT_EQ(true_value, node.field);
    fdd::leaf pos = boost::get<fdd::leaf>(node.positive);
    EXPECT_THAT(pos.sets, UnorderedElementsAre(
        oxm::field_set{F<3>() == 3},
        oxm::field_set() // filter empty value
    ));
    fdd::leaf neg = boost::get<fdd::leaf>(node.negative);
    EXPECT_THAT(neg.sets, UnorderedElementsAre(oxm::field_set{F<3>() == 3}));
}

TEST(FddTest, ParallellLeafNode) {
    policy p = modify(F<3>() << 3) + filter(F<1>() == 1);
    fdd::diagram diagram = fdd::compile(p);
    fdd::node node = boost::get<fdd::node>(diagram);
    oxm::field<> true_value = F<1>() == 1;
    EXPECT_EQ(true_value, node.field);
    fdd::leaf pos = boost::get<fdd::leaf>(node.positive);
    EXPECT_THAT(pos.sets, UnorderedElementsAre(
        oxm::field_set{F<3>() == 3},
        oxm::field_set() // filter empty value
    ));
    fdd::leaf neg = boost::get<fdd::leaf>(node.negative);
    EXPECT_THAT(neg.sets, UnorderedElementsAre(oxm::field_set{F<3>() == 3}));
}

TEST(FddTest, ParallelNodeNodeEquals) {
    fdd::diagram node1 = fdd::node{
            {F<1>() == 1},
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };
    fdd::diagram node2 = fdd::node{
            {F<1>() == 1},
            fdd::leaf{{oxm::field_set{F<5>() == 5}}},
            fdd::leaf{{oxm::field_set{F<6>() == 6}}}
    };
    fdd::parallel_composition pc;
    fdd::diagram diagram = boost::apply_visitor(pc, node1, node2);
    fdd::node node = boost::get<fdd::node>(diagram);
    EXPECT_EQ(oxm::field<>(F<1>() == 1), node.field);
    fdd::leaf pos = boost::get<fdd::leaf>(node.positive);
    EXPECT_THAT(pos.sets, UnorderedElementsAre(
        oxm::field_set{F<2>() == 2},
        oxm::field_set{F<5>() == 5}
    ));
    fdd::leaf neg = boost::get<fdd::leaf>(node.negative);
    EXPECT_THAT(neg.sets, UnorderedElementsAre(
        oxm::field_set{F<3>() == 3},
        oxm::field_set{F<6>() == 6}
    ));
}

TEST(FddTest, ParallelNodeNodeEqualFields) {
    fdd::diagram node1 = fdd::node{
            {F<1>() == 1},
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };
    fdd::diagram node2 = fdd::node{
            {F<1>() == 2},
            fdd::leaf{{oxm::field_set{F<5>() == 5}}},
            fdd::leaf{{oxm::field_set{F<6>() == 6}}}
    };
    fdd::parallel_composition pc;
    fdd::diagram result_value1 = boost::apply_visitor(pc, node1, node2);
    fdd::diagram result_value2 =  boost::apply_visitor(pc, node2, node1);

    fdd::diagram true_value = fdd::node {
        {F<1>() == 1},
        fdd::leaf {{
            oxm::field_set{F<2>() == 2},
            oxm::field_set{F<6>() == 6}
        }},
        fdd::node{
            {F<1>() == 2},
            fdd::leaf{{
                oxm::field_set{F<3>() == 3},
                oxm::field_set{F<5>() == 5}
            }},
            fdd::leaf{{
                oxm::field_set{F<6>() == 6},
                oxm::field_set{F<3>() == 3}
            }}
        }
    };


    EXPECT_EQ(true_value, result_value1);
    EXPECT_EQ(true_value, result_value2);

}

TEST(FddTest, ParallelNodeNodeDiffFields) {
    fdd::diagram node1 = fdd::node{
            {F<1>() == 1},
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };
    fdd::diagram node2 = fdd::node{
            {F<2>() == 2},
            fdd::leaf{{oxm::field_set{F<5>() == 5}}},
            fdd::leaf{{oxm::field_set{F<6>() == 6}}}
    };
    fdd::parallel_composition pc;
    fdd::diagram result_value1 = boost::apply_visitor(pc, node1, node2);
    fdd::diagram result_value2 =  boost::apply_visitor(pc, node2, node1);

    fdd::diagram true_value = fdd::node {
        {F<1>() == 1},
        fdd::node {
            {F<2>() == 2},
            fdd::leaf{{
                oxm::field_set{F<2>() == 2},
                oxm::field_set{F<5>() == 5}
            }},
            fdd::leaf{{
                oxm::field_set{F<2>() == 2},
                oxm::field_set{F<6>() == 6}
            }}
        },
        fdd::node{
            {F<2>() == 2},
            fdd::leaf{{
                oxm::field_set{F<3>() == 3},
                oxm::field_set{F<5>() == 5}
            }},
            fdd::leaf{{
                oxm::field_set{F<3>() == 3},
                oxm::field_set{F<6>() == 6}
            }}
        }
    };


    EXPECT_EQ(true_value, result_value1);
    EXPECT_EQ(true_value, result_value2);
}


TEST(FddTest, RestrictionLeafTrue) {
    fdd::diagram d = fdd::leaf{{oxm::field_set{F<2>() == 2}}};
    auto r = fdd::restriction{ F<1>() == 1, d, true };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeEqualTrue) {
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };


    auto r = fdd::restriction{ F<1>() == 1, d, true };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeEqualTypesTrue) {
    fdd::diagram d = fdd::node {
        F<1>() == 2,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<1>() == 1, d, true };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<3>() == 3}}},
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeDiffTypes1Less2True) {
    fdd::diagram d = fdd::node {
        F<2>() == 2,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<1>() == 1, d, true };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::node {
            F<2>() == 2,
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
        },
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeOtherwiseTrue) {
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<2>() == 2, d, true };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::node {
            F<2>() == 2,
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{}
        },
        fdd::node {
            F<2>() == 2,
            fdd::leaf{{oxm::field_set{F<3>() == 3}}},
            fdd::leaf{}
        },
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionLeafFalse) {
    fdd::diagram d = fdd::leaf{{oxm::field_set{F<2>() == 2}}};
    auto r = fdd::restriction{ F<1>() == 1, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::leaf{{oxm::field_set{F<2>() == 2}}}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeEqualFalse) {
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };


    auto r = fdd::restriction{ F<1>() == 1, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeEqualTypesFalseOrdering1) {
    fdd::diagram d = fdd::node {
        F<1>() == 2,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<1>() == 1, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::node {
            F<1>() == 2,
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
        }
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeEqualTypesFalseOrdering2) {
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<1>() == 2, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::node {
            F<1>() == 2,
            fdd::leaf{},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
        }
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeDiffTypesFalseOrdering1) {
    fdd::diagram d = fdd::node {
        F<2>() == 2,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<1>() == 1, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::node {
            F<2>() == 2,
            fdd::leaf{{oxm::field_set{F<2>() == 2}}},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
        }
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, RestrictionNodeDiffTypesFalseOrdering2) {
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    auto r = fdd::restriction{ F<2>() == 2, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::node {
            F<2>() == 2,
            fdd::leaf{},
            fdd::leaf{{oxm::field_set{F<2>() == 2}}}
        },
        fdd::node {
            F<2>() == 2,
            fdd::leaf{},
            fdd::leaf{{oxm::field_set{F<3>() == 3}}}
        }
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddTest, SequentialLeafLeaf) {
    fdd::diagram d1 = fdd::leaf {{
        oxm::field_set{F<1>() == 1}
    }};

    fdd::diagram d2 = fdd::leaf {{
        oxm::field_set{F<2>() == 2}
    }};

    fdd::sequential_composition composition;
    fdd::diagram result = boost::apply_visitor(composition, d1,  d2);
    fdd::diagram true_value = fdd::leaf {{
        oxm::field_set{
            {F<1>() == 1},
            {F<2>() == 2}
        }
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, SequentialLeafLeafOneField) {
    fdd::diagram d1 = fdd::leaf {{
        oxm::field_set{F<1>() == 1}
    }};

    fdd::diagram d2 = fdd::leaf {{
        oxm::field_set{F<1>() == 2}
    }};

    fdd::sequential_composition composition;
    fdd::diagram result = boost::apply_visitor(composition, d1,  d2);
    fdd::diagram true_value = fdd::leaf {{
        oxm::field_set{
            {F<1>() == 2}
        }
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, SequentialLeafLeafMulti) {
    fdd::diagram d1 = fdd::leaf {{
        oxm::field_set{F<1>() == 1},
        oxm::field_set{F<3>() == 3},
        oxm::field_set{F<4>() == 4}
    }};

    fdd::diagram d2 = fdd::leaf {{
        oxm::field_set{F<2>() == 2},
        oxm::field_set{F<3>() == 300},
    }};

    fdd::sequential_composition composition;
    fdd::diagram result = boost::apply_visitor(composition, d1,  d2);
    fdd::diagram true_value = fdd::leaf {{
        oxm::field_set{
            {F<1>() == 1},
            {F<2>() == 2}
        },
        oxm::field_set{
            {F<1>() == 1},
            {F<3>() == 300}
        },
        oxm::field_set{
            {F<3>() == 3},
            {F<2>() == 2}
        },
        oxm::field_set{
            {F<3>() == 300}
        },
        oxm::field_set{
            {F<4>() == 4},
            {F<2>() == 2}
        },
        oxm::field_set{
            {F<4>() == 4},
            {F<3>() == 300}
        }
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, SequentialLeafNodeWriteThisValue) {
    fdd::diagram d1 = fdd::leaf{{
        oxm::field_set{F<1>() == 1}
    }};

    fdd::diagram d2 = fdd::node{
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    fdd::diagram result = boost::apply_visitor(fdd::sequential_composition{}, d1, d2);

    fdd::diagram true_value = fdd::leaf {{
        oxm::field_set{
            {F<1>() == 1},
            {F<2>() == 2}
        }
    }};
    EXPECT_EQ(true_value, result);

}

TEST(FddTest, SequentialLeafNodeWriteAnotherValue) {
    fdd::diagram d1 = fdd::leaf{{
        oxm::field_set{F<1>() == 100}
    }};

    fdd::diagram d2 = fdd::node{
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    fdd::diagram result = boost::apply_visitor(fdd::sequential_composition{}, d1, d2);

    fdd::diagram true_value = fdd::leaf {{
        oxm::field_set{
            {F<1>() == 100},
            {F<3>() == 3}
        }
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, SequentialLeafNodeWriteAnotherType) {
    fdd::diagram d1 = fdd::leaf{{
        oxm::field_set{F<100>() == 100}
    }};

    fdd::diagram d2 = fdd::node{
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    fdd::diagram result = boost::apply_visitor(fdd::sequential_composition{}, d1, d2);

    fdd::diagram true_value = fdd::node{
        F<1>() == 1,
        fdd::leaf{{
            oxm::field_set{ {F<2>() == 2}, {F<100>() == 100} }
        }},
        fdd::leaf{{
            oxm::field_set{ {F<3>() == 3}, {F<100>() == 100} }
        }}
    };
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, SequentialNodeLeaf) {
    fdd::diagram d1 = fdd::node{
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{oxm::field_set{F<3>() == 3}}}
    };

    fdd::diagram d2 = fdd::leaf{{
        oxm::field_set{F<100>() == 100}
    }};

    fdd::diagram result = boost::apply_visitor(fdd::sequential_composition{}, d1, d2);

    fdd::diagram true_value = fdd::node{
        F<1>() == 1,
        fdd::leaf{{
            oxm::field_set{ {F<2>() == 2}, {F<100>() == 100} }
        }},
        fdd::leaf{{
            oxm::field_set{ {F<3>() == 3}, {F<100>() == 100} }
        }}
    };
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, CompileSequential) {
    policy p = modify(F<1>() == 1) >> modify(F<2>() == 2);
    fdd::diagram result = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{
        oxm::field_set{ {F<1>() == 1}, {F<2>() == 2} }
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddTest, CompileParallel) {
    policy p = modify(F<1>() == 1) + modify(F<2>() == 2);
    fdd::diagram result = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{
        oxm::field_set{F<1>() == 1},
        oxm::field_set{F<2>() == 2}
    }};
    EXPECT_EQ(true_value, result);
}

TEST(DISABLED_FddTest, CompilePacketFunction) {
    policy p = handler([](Packet& pkt){ return stop(); });
    fdd::diagram result = fdd::compile(p);
    EXPECT_TRUE(false) << "You need implement it!";
}

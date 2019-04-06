#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.hh"

#include "retic/fdd.hh"
#include "retic/fdd_compiler.hh"
#include "retic/policies.hh"
#include "retic/traverse_fdd.hh"
#include "oxm/openflow_basic.hh"

using namespace runos;
using namespace retic;
using namespace ::testing;

using sec = std::chrono::seconds;

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

TEST(EqualFdd, EqualWithFlowSettings) {
    fdd::diagram d1 = fdd::leaf{{oxm::field_set{}}, FlowSettings{sec(10), sec(20)}};
    fdd::diagram d2 = fdd::leaf{{oxm::field_set{}}, FlowSettings{sec(10), sec(20)}};
    fdd::diagram d3 = fdd::leaf{{oxm::field_set{}}, FlowSettings{sec(1234), sec(20)}};
    fdd::diagram d4 = fdd::leaf{{oxm::field_set{}}, FlowSettings{sec(10), sec(1234)}};
    EXPECT_EQ(d1, d2);
    EXPECT_NE(d1, d3);
    EXPECT_NE(d1, d4);
}


TEST(FddCompilerTest, StopCompile) {
    policy p = stop();
    fdd::diagram diagram = fdd::compile(p);
    fdd::leaf leaf = boost::get<fdd::leaf>(diagram);
    ASSERT_TRUE(leaf.sets.empty());
}

TEST(FddCompilerTest, IdCompile) {
    policy p = id();
    fdd::diagram diagram = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{ oxm::field_set{} }};
    ASSERT_EQ(diagram, true_value);
}

TEST(FddCompilerTest, Modify) {
    policy p = modify(F<1>() << 100);
    fdd::diagram diagram = fdd::compile(p);
    fdd::leaf leaf = boost::get<fdd::leaf>(diagram);
    ASSERT_THAT(leaf.sets, SizeIs(1));
    ASSERT_EQ(oxm::field_set{F<1>() == 100}, leaf.sets[0]);
}

TEST(FddCompilerTest, Filter) {
    policy p = filter(F<1>() == 100);
    fdd::diagram diagram = fdd::compile(p);
    fdd::node node = boost::get<fdd::node>(diagram);
    fdd::diagram true_value = fdd::node {
         F<1>() == 100,
         fdd::leaf {{ oxm::field_set{} }},
         fdd::leaf { }
    };
    ASSERT_EQ(true_value, diagram);

}

TEST(FddCompilerTest, ParallelLeafLeaf) {
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

TEST(FddCompilerTest, ParallellNodeLeaf) {
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

TEST(FddCompilerTest, ParallellLeafNode) {
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

TEST(FddCompilerTest, ParallelNodeNodeEquals) {
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

TEST(FddCompilerTest, ParallelNodeNodeEqualFields) {
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

TEST(FddCompilerTest, ParallelNodeNodeDiffFields) {
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

TEST(FddCompilerTest, RestrictionLeafTrue) {
    fdd::diagram d = fdd::leaf{{oxm::field_set{F<2>() == 2}}};
    auto r = fdd::restriction{ F<1>() == 1, d, true };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 2}}},
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddCompilerTest, RestrictionNodeEqualTrue) {
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

TEST(FddCompilerTest, RestrictionNodeEqualTypesTrue) {
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

TEST(FddCompilerTest, RestrictionNodeDiffTypes1Less2True) {
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

TEST(FddCompilerTest, RestrictionNodeOtherwiseTrue) {
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

TEST(FddCompilerTest, RestrictionLeafFalse) {
    fdd::diagram d = fdd::leaf{{oxm::field_set{F<2>() == 2}}};
    auto r = fdd::restriction{ F<1>() == 1, d, false };
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::leaf{{oxm::field_set{F<2>() == 2}}}
    };
    EXPECT_EQ(true_value, r.apply());
}

TEST(FddCompilerTest, RestrictionNodeEqualFalse) {
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

TEST(FddCompilerTest, RestrictionNodeEqualTypesFalseOrdering1) {
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

TEST(FddCompilerTest, RestrictionNodeEqualTypesFalseOrdering2) {
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

TEST(FddCompilerTest, RestrictionNodeDiffTypesFalseOrdering1) {
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

TEST(FddCompilerTest, RestrictionNodeDiffTypesFalseOrdering2) {
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

TEST(FddCompilerTest, SequentialLeafLeaf) {
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

TEST(FddCompilerTest, SequentialLeafLeafOneField) {
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

TEST(FddCompilerTest, SequentialLeafLeafMulti) {
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

TEST(FddCompilerTest, SequentialLeafNodeWriteThisValue) {
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

TEST(FddCompilerTest, SequentialLeafNodeWriteAnotherValue) {
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

TEST(FddCompilerTest, SequentialLeafNodeWriteAnotherType) {
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

TEST(FddCompilerTest, SequentialNodeLeaf) {
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

TEST(FddCompilerTest, CompileSequential) {
    policy p = modify(F<1>() == 1) >> modify(F<2>() == 2);
    fdd::diagram result = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{
        oxm::field_set{ {F<1>() == 1}, {F<2>() == 2} }
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddCompilerTest, CompileParallel) {
    policy p = modify(F<1>() == 1) + modify(F<2>() == 2);
    fdd::diagram result = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{
        oxm::field_set{F<1>() == 1},
        oxm::field_set{F<2>() == 2}
    }};
    EXPECT_EQ(true_value, result);
}

TEST(FddCompilerTest, CompilePacketFunction) {
    policy p = handler([](Packet& pkt){ return stop(); });
    PacketFunction pf = boost::get<PacketFunction>(p);
    fdd::diagram result = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{ {oxm::field_set{}, pf} }};
    ASSERT_THAT(true_value, result);
}

TEST(FddCompilerTest, SeqAction1If) {
    policy p1 = handler([](Packet& pkt){ return stop(); });
    policy p2 = handler([](Packet& pkt){ return fwd(1); });
    auto pf1 = boost::get<PacketFunction>(p1);
    auto pf2 = boost::get<PacketFunction>(p2);
    fdd::diagram d1 = fdd::leaf{{ {oxm::field_set{F<1>() == 1}, pf1} }};
    fdd::diagram d2 = fdd::leaf{{ {oxm::field_set{F<2>() == 2}, pf2} }};
    fdd::diagram result = boost::apply_visitor(fdd::sequential_composition{}, d1, d2);
    fdd::action_unit au = fdd::action_unit{oxm::field_set{F<2>() == 2}, pf2};
    fdd::diagram true_value = fdd::leaf{{ {oxm::field_set{F<1>() == 1}, pf1, au}}};
    ASSERT_EQ(true_value, result);
}

TEST(FddCompilerTest, SeqAction2If) {
    policy p2 = handler([](Packet& pkt){ return fwd(1); });
    auto pf2 = boost::get<PacketFunction>(p2);
    fdd::diagram d1 = fdd::leaf{{ {oxm::field_set{F<1>() == 1}} }};
    fdd::diagram d2 = fdd::leaf{{ {oxm::field_set{F<2>() == 2}, pf2} }};
    fdd::diagram result = boost::apply_visitor(fdd::sequential_composition{}, d1, d2);
    fdd::diagram true_value = fdd::leaf{{ {oxm::field_set{F<1>() == 1, F<2>() == 2}, pf2}}};
    ASSERT_EQ(true_value, result);
}

TEST(FddCompilerTest, FlowSettings) {
    policy p = idle_timeout(sec(10));
    auto settings = boost::get<FlowSettings>(p);
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{oxm::field_set{}}, settings};
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, FlowSettingsSeq) {
    policy p = idle_timeout(sec(10)) >> idle_timeout(sec(5));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{oxm::field_set{}}, FlowSettings{.idle_timeout = sec(5)}};
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, FlowSettingsSeq2) {
    policy p = idle_timeout(sec(5)) >> idle_timeout(sec(10));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{oxm::field_set{}}, FlowSettings{.idle_timeout = sec(5)}};
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, FlowSettingsParallel) {
    policy p = idle_timeout(sec(10)) + idle_timeout(sec(5));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf{{oxm::field_set{}}, FlowSettings{.idle_timeout = sec(5)}};
    EXPECT_EQ(true_value, d);
}


TEST(FddCompilerTest, FlowSettingsWithTwoActions) {
    policy p = (modify(F<1>() == 1) >> idle_timeout(sec(20))) + 
               (modify(F<2>() == 2) >> idle_timeout(sec(30)));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::leaf {
        {
            oxm::field_set{F<1>() == 1},
            oxm::field_set{F<2>() == 2}
        },
        FlowSettings{.idle_timeout = sec(20)}
    };
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, FilterFlowSettings) {
    policy p = filter(F<1>() == 1) >> idle_timeout(sec(20));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{}}, FlowSettings{.idle_timeout=sec(20)}},
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, NegationTest) {
    policy p = filter_not(F<1>() == 1);
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::leaf{{oxm::field_set{}}}
    };
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, NegationTestWithSettings) {
    policy p = filter_not(F<1>() == 1) >> idle_timeout(sec(20));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{},
        fdd::leaf{{oxm::field_set{}}, FlowSettings{.idle_timeout=sec(20)}}
    };
    EXPECT_EQ(true_value, d);
}

TEST(DISABLED_FddCompilerTest, NegationX2TestWithSettings) {
    // I think this test should failed.
    // But I have no idea why NegationTestWithSettings is works
    //
    policy p = filter_not(F<1>() == 1) >> (filter_not(F<1>() == 1) >> idle_timeout(sec(20)));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{}}, FlowSettings{.idle_timeout=sec(20)}},
        fdd::leaf{}
    };
    EXPECT_EQ(true_value, d);
}

TEST(FddCompilerTest, ComplexNegationTest) {
    policy p = (filter(F<1>() == 1) >> modify(F<2>() == 1)) +
               (filter_not(F<1>() == 1) >> modify(F<2>() == 2));
    fdd::diagram d = fdd::compile(p);
    fdd::diagram true_value = fdd::node {
        F<1>() == 1,
        fdd::leaf{{oxm::field_set{F<2>() == 1}}},
        fdd::leaf{{oxm::field_set{F<2>() == 2}}}
    };
    EXPECT_EQ(true_value, d);
}

TEST(FddTraverseTest, FddTraverse) {
    fdd::diagram d = fdd::node{
        F<1>() == 1,
        fdd::leaf{{ oxm::field_set{F<2>() == 2}}},
        fdd::leaf{{ oxm::field_set{F<3>() == 3}}}
    };
    oxm::field_set fs1{F<1>() == 1};
    oxm::field_set fs2{F<1>() == 2};
    fdd::Traverser traverser1{fs1}, traverser2{fs2};
    fdd::leaf l1 = boost::apply_visitor(traverser1, d);
    fdd::leaf l2 = boost::apply_visitor(traverser2, d);
    EXPECT_EQ(l1, fdd::leaf{{ oxm::field_set{F<2>() == 2}}});
    EXPECT_EQ(l2, fdd::leaf{{ oxm::field_set{F<3>() == 3}}});
}

TEST(FddTraverseTest, FddTraverseWithMaple) {
    policy p = handler([](Packet& pkt) {
        // Test with nested function
        return handler([](Packet& pkt) {
            return modify(F<2>() << 2);
        });
    });
    auto pf = boost::get<PacketFunction>(p);
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{ {oxm::field_set{}, pf} }},
        fdd::leaf{}
    };
    oxm::field_set fs{F<1>() == 1};
    fdd::Traverser traverser{fs};
    fdd::leaf& l = boost::apply_visitor(traverser, d);
    EXPECT_EQ(l, fdd::leaf{{ oxm::field_set{F<2>() == 2} }});
}

TEST(FddTraverseTest, FddTraverseWithMapleCallMeTwice) {
    int call_count = 0;
    policy p = handler([&call_count](Packet& pkt) {
        call_count++;
        return hard_timeout(duration::zero());
    });

    oxm::field_set fs{F<1>() == 1};
    fdd::diagram d = fdd::compile(p);
    for (int i = 0; i < 2; i++) {
        fdd::Traverser traverser{fs};
        boost::apply_visitor(traverser, d);
    }
    EXPECT_EQ(call_count, 2);
}

using match = std::vector<oxm::field_set>;

TEST(FddTraverseTest, FddTraverseWithMapleWithBackend) {
    MockBackend backend;

    policy p = handler([](Packet& pkt) {
        pkt.test(F<1>() == 1); // should't be called becouse of cache
        pkt.test(F<3>() == 3); // should be called
        return modify(F<2>() << 2);
    });
    auto pf = boost::get<PacketFunction>(p);
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{ {oxm::field_set{}, pf} }},
        fdd::leaf{}
    };
    oxm::field_set fs{F<1>() == 1, F<2>() == 2, F<3>() == 3};
    fdd::Traverser traverser{fs, &backend};
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1, F<3>() == 3}, _)).Times(1);
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1}, _)).Times(0);

    EXPECT_CALL(
        backend,
        install(
            oxm::field_set{F<1>() == 1, F<3>() == 3},
            match{oxm::field_set{F<2>() == 2}},
            _, _
        )
    ).Times(1);

    fdd::leaf& l = boost::apply_visitor(traverser, d);
    EXPECT_EQ(l, fdd::leaf{{ oxm::field_set{F<2>() == 2} }});
}

TEST(FddTraverseTest, FddTraverseWithMapleWithBackendNestedFunction) {
    MockBackend backend;

    policy p = handler([](Packet& pkt) {
        // Test with nested function
        pkt.test(F<1>() == 1); // should't be called becouse of cache
        pkt.test(F<3>() == 3); // should be called
        return handler([](Packet& pkt) {
            return modify(F<2>() << 2);
        });
    });
    auto pf = boost::get<PacketFunction>(p);
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{ {oxm::field_set{}, pf} }},
        fdd::leaf{}
    };
    oxm::field_set fs{F<1>() == 1, F<2>() == 2, F<3>() == 3};
    fdd::Traverser traverser{fs, &backend};
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1, F<3>() == 3}, _)).Times(Between(1,2));
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1}, _)).Times(0);

    EXPECT_CALL(
        backend,
        install(
            oxm::field_set{F<1>() == 1, F<3>() == 3},
            match{oxm::field_set{F<2>() == 2}},
            _, _
        )
    ).Times(1);

    fdd::leaf& l = boost::apply_visitor(traverser, d);
    EXPECT_EQ(l, fdd::leaf{{ oxm::field_set{F<2>() == 2} }});
}

TEST(DISABLED_FddCompilerTest, Broadcast) {
    static constexpr auto switch_id = oxm::switch_id();
    policy p = (filter(switch_id == 2) >> stop()) +
               (filter(switch_id == 3) >> stop()) +
               (filter(switch_id == 4) >> stop()) +
               (filter(switch_id == 1) >> stop());
    fdd::diagram true_value = fdd::node{
        switch_id == 1,
        fdd::leaf{},
        fdd::node{
            switch_id == 2,
            fdd::leaf{},
            fdd::node{
                switch_id == 3,
                fdd::leaf{},
                fdd::node{
                    switch_id == 4,
                    fdd::leaf{},
                    fdd::leaf{}
                }
            }
        }
    };
    fdd::diagram result_value = fdd::compile(p);
    ASSERT_EQ(true_value, result_value);
}

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/variant/apply_visitor.hpp>

#include "retic/tracer.hh"
#include "retic/trace_tree.hh"
#include "retic/trace_tree_translator.hh"
#include "retic/backend.hh"
#include "retic/fdd.hh"
#include "retic/fdd_compiler.hh"
#include "retic/policies.hh"
#include "retic/traverse_trace_tree.hh"
#include "retic/leaf_applier.hh"
#include "oxm/openflow_basic.hh"

using namespace runos;
using namespace retic;
using namespace trace_tree;
using namespace ::testing;

template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };

struct MockBackend: public Backend {
    MOCK_METHOD3(install, void(oxm::field_set, std::vector<oxm::field_set>, uint16_t));
    MOCK_METHOD2(installBarrier, void(oxm::field_set, uint16_t));
};

using match = std::vector<oxm::field_set>;

TEST(TraceTreeTest, CreateTraceTreeLoadMethod) {
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root);
    oxm::field<> f = F<1>() == 1;
    tracer::trace_node n = tracer::load_node{f};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::load_node{oxm::mask<>(f), {{f.value_bits(), trace_tree::unexplored{}}}};
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 = 
        trace_tree::load_node{
            oxm::mask<>(f), {{
                f.value_bits(), trace_tree::load_node{
                    oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
                }
            }}
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, CreateTraceTreeLoadMethodOnEmptyLoadNode) {
    oxm::field<> f = F<1>() == 1;
    trace_tree::node root = trace_tree::load_node{oxm::mask<>(f), {}};
    trace_tree::Augmention augmenter(&root);
    tracer::trace_node n = tracer::load_node{f};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::load_node{oxm::mask<>(f), {{f.value_bits(), trace_tree::unexplored{}}}};
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 = 
        trace_tree::load_node{
            oxm::mask<>(f), {{
                f.value_bits(), trace_tree::load_node{
                    oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
                }
            }}
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, CreateTraceTreeLoadMethodOnLoadNode) {
    oxm::field<> f = F<1>() == 1, g = F<1>() == 2;
    trace_tree::node root = load_node{oxm::mask<>(f), {{g.value_bits(), unexplored{}}}};
    trace_tree::Augmention augmenter(&root);
    tracer::trace_node n = tracer::load_node{f};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value =
        trace_tree::load_node{oxm::mask<>(f),
        {
            {f.value_bits(), trace_tree::unexplored{}},
            {g.value_bits(), trace_tree::unexplored{}}
        }
    };
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 = 
        trace_tree::load_node{
            oxm::mask<>(f), {
                {
                    f.value_bits(), trace_tree::load_node{
                        oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
                    }
                },
                { g.value_bits(), trace_tree::unexplored{} }
            }
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, CreateTraceTreeTestMethodPositive) {
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root);
    oxm::field<> f = F<1>() == 1;
    tracer::trace_node n = tracer::test_node{f, true};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::test_node{f, trace_tree::unexplored{}, trace_tree::unexplored{}};
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 =
        trace_tree::test_node{
            f,
            trace_tree::load_node{
                    oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
            },
            trace_tree::unexplored{}
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, CreateTraceTreeTestMethodNegative) {
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root);
    oxm::field<> f = F<1>() == 1;
    tracer::trace_node n = tracer::test_node{f, false};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::test_node{f, trace_tree::unexplored{}, trace_tree::unexplored{}};
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 =
        trace_tree::test_node{
            f,
            trace_tree::unexplored{},
            trace_tree::load_node{
                    oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
            }
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, CreateTraceTreeTestMethodPositiveOnExisting) {
    oxm::field<> f = F<1>() == 1;
    trace_tree::node root = trace_tree::test_node{f, trace_tree::unexplored{}, trace_tree::unexplored{}};
    trace_tree::Augmention augmenter(&root);
    tracer::trace_node n = tracer::test_node{f, true};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::test_node{f, trace_tree::unexplored{}, trace_tree::unexplored{}};
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 =
        trace_tree::test_node{
            f,
            trace_tree::load_node{
                    oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
            },
            trace_tree::unexplored{}
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, CreateTraceTreeTestMethodNegativeOnExisting) {
    oxm::field<> f = F<1>() == 1;
    trace_tree::node root = trace_tree::test_node{f, trace_tree::unexplored{}, trace_tree::unexplored{}};
    trace_tree::Augmention augmenter(&root);
    tracer::trace_node n = tracer::test_node{f, false};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::test_node{f, trace_tree::unexplored{}, trace_tree::unexplored{}};
    ASSERT_EQ(root, true_value);

    oxm::field<> f2 = F<2>() == 2;
    tracer::trace_node n2 = tracer::load_node{f2};
    boost::apply_visitor(augmenter, n2);
    trace_tree::node true_value2 =
        trace_tree::test_node{
            f,
            trace_tree::unexplored{},
            trace_tree::load_node{
                    oxm::mask<>(f2), {{f2.value_bits(), trace_tree::unexplored{}}}
            }
        };
    ASSERT_EQ(true_value2, root) << "Current pointer doesn't change";
}

TEST(TraceTreeTest, FinishOnEmpty) {
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root);
    policy p = modify(F<1>() << 1);
    augmenter.finish(p);
    trace_tree::node true_value = trace_tree::leaf_node{p};
    ASSERT_EQ(root, true_value);
}

TEST(TraceTreeTest, FinishOnExisting) {
    trace_tree::node root = trace_tree::leaf_node{ modify(F<2>() << 2) };
    trace_tree::Augmention augmenter(&root);
    policy p = modify(F<1>() << 1);
    augmenter.finish(p);
    trace_tree::node true_value = trace_tree::leaf_node{p};
    ASSERT_EQ(root, true_value);
}

TEST(OnLineTranslation, LeafAction) {
    MockBackend backend;
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root, &backend, oxm::field_set{}, 10, 200);
    EXPECT_CALL(backend, install(oxm::field_set{}, match{oxm::field_set{F<1>() == 1}}, _));
    augmenter.finish(modify(F<1>() << 1));
}

TEST(OnLineTranslation, LoadMethod) {
    MockBackend backend;
    trace_tree::node root = trace_tree::unexplored{};
    tracer::trace_node n = tracer::load_node{F<1>() == 2};
    trace_tree::Augmention augmenter(&root, &backend, oxm::field_set{}, 10, 200);
    boost::apply_visitor(augmenter, n);
    EXPECT_CALL(backend, install(oxm::field_set{F<1>() == 2}, match{oxm::field_set{F<1>() == 1}}, _));
    augmenter.finish(modify(F<1>() << 1));
}

TEST(OnLineTranslation, TestMethod) {
    MockBackend backend;
    uint16_t barrier_prio, positive_prio, negative_prio;
    trace_tree::node root = trace_tree::unexplored{};
    tracer::trace_node positive_n = tracer::test_node{F<1>() == 2, true};
    trace_tree::Augmention augmenter(&root, &backend, oxm::field_set{}, 10, 200);
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 2}, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&barrier_prio));

    boost::apply_visitor(augmenter, positive_n);

    EXPECT_CALL(backend, install(oxm::field_set{F<1>() == 2}, match{oxm::field_set{F<1>() == 1}}, _))
        .WillOnce(SaveArg<2>(&positive_prio));
    augmenter.finish(modify(F<1>() << 1));

    EXPECT_CALL(backend, installBarrier(_, _)).Times(0);
    EXPECT_CALL(backend, install(oxm::field_set{}, match{oxm::field_set{F<2>() == 2}}, _))
        .WillOnce(SaveArg<2>(&negative_prio));

    tracer::trace_node negative_n = tracer::test_node({F<1>() == 2, false});
    trace_tree::Augmention negative_augmenter(&root, &backend, oxm::field_set{}, 10, 200);
    boost::apply_visitor(negative_augmenter, negative_n);
    negative_augmenter.finish(modify(F<2>() << 2));

    EXPECT_LT(negative_prio, barrier_prio);
    EXPECT_LT(barrier_prio, positive_prio);
}

TEST(AugmentionFinalReturnValue, ReturnValue) {
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root);
    policy p = (filter(F<1>() == 1) >> modify(F<2>() == 2)) + (filter(F<3>() == 3) >> modify(F<3>() << 2));
    auto result = augmenter.finish(p);
    fdd::diagram true_value = fdd::compile(p);
    EXPECT_EQ(true_value, result->value);

}

TEST(TraverseTraceTree, Unexplored) {
    trace_tree::node root = trace_tree::unexplored{};
    oxm::field_set fs;
    trace_tree::Traverser traverser(fs);
    auto [res, match] = boost::apply_visitor(traverser, root);
    EXPECT_EQ(nullptr, res);
}

TEST(TraverseTraceTree, Load) {
    oxm::field<> f = F<1>() == 1;
    fdd::diagram d = fdd::leaf{};
    auto dh = std::make_shared<fdd::diagram_holder>();
    dh->value = d;
    trace_tree::node root = trace_tree::load_node{
        oxm::mask<>(f),
        {
            {f.value_bits(), trace_tree::leaf_node{stop(), dh}}
        }
    };
    oxm::field_set fs1 = oxm::field_set{F<1>() == 1};
    oxm::field_set fs2 = oxm::field_set{F<1>() == 2};

    trace_tree::Traverser traverser1(fs1);
    auto [res1, match1] = boost::apply_visitor(traverser1, root);
    EXPECT_EQ(dh, res1);
    EXPECT_EQ(match1, oxm::field_set{F<1>() == 1});

    trace_tree::Traverser traverser2(fs2);
    auto [res2, match2] = boost::apply_visitor(traverser2, root);
    EXPECT_EQ(nullptr, res2);

}

TEST(TraverseTraceTree, Test) {
    oxm::field<> f = F<1>() == 1;

    fdd::diagram d_pos = fdd::leaf{};
    auto dh_pos = std::make_shared<fdd::diagram_holder>();
    dh_pos->value = d_pos;

    fdd::diagram d_neg = fdd::leaf{};
    auto dh_neg = std::make_shared<fdd::diagram_holder>();
    dh_neg->value = d_neg;

    trace_tree::node root = trace_tree::test_node{
        f,
        trace_tree::leaf_node{stop(), dh_pos},
        trace_tree::leaf_node{stop(), dh_neg}
    };
    oxm::field_set fs1 = oxm::field_set{F<1>() == 1};
    oxm::field_set fs2 = oxm::field_set{F<1>() == 2};

    trace_tree::Traverser traverser1(fs1);
    auto [res1, match1] = boost::apply_visitor(traverser1, root);
    EXPECT_EQ(dh_pos, res1);
    EXPECT_EQ(match1, oxm::field_set{F<1>() == 1});

    trace_tree::Traverser traverser2(fs2);
    auto [res2, match2] = boost::apply_visitor(traverser2, root);
    EXPECT_EQ(dh_neg, res2);
    EXPECT_EQ(match2, oxm::field_set{});
}

TEST(TraceTreeComplex, GetTracesMergeAndAugment) {
    MockBackend backend;
    trace_tree::node root = trace_tree::unexplored{};
    policy p = handler([](Packet& pkt) {
        // Test with nested function
        pkt.test(F<1>() == 1); // should't be called becouse of cache
        pkt.test(F<3>() == 3); // should be called
        return modify(F<2>() << 2);
    });
    auto pf = boost::get<PacketFunction>(p);
    fdd::leaf l = fdd::leaf{{ {oxm::field_set{}, pf} }};
    oxm::field_set fs{F<1>() == 1, F<2>() == 2, F<3>() == 3};

    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1, F<3>() == 3}, _)).Times(1);
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1}, _)).Times(0);

    EXPECT_CALL(
        backend,
        install(
            oxm::field_set{F<1>() == 1, F<3>() == 3},
            match{oxm::field_set{F<2>() == 2}},
            _
        )
    ).Times(1);

    auto traces = retic::getTraces(l, fs);
    auto merged_trace = tracer::mergeTrace(traces, oxm::field_set{F<1>() == 1});
    trace_tree::Augmention augmenter( &root, &backend, oxm::field_set{F<1>() == 1}, 1, 1000);
    for (auto& n: merged_trace.values()) {
        boost::apply_visitor(augmenter, n);
    }
    augmenter.finish(merged_trace.result());
}

TEST(TraceTreeComplex, GetTracesMergeAndAugmentNestedFunction) {
    MockBackend backend;
    trace_tree::node root = trace_tree::unexplored{};
    policy nested_handler = handler([](Packet& pkt) {
            return modify(F<2>() << 2);
    });
    fdd::diagram true_next_fdd = fdd::compile(nested_handler);

    policy p = handler([nested_handler](Packet& pkt) {
        // Test with nested function
        pkt.test(F<1>() == 1); // should't be called becouse of cache
        pkt.test(F<3>() == 3); // should be called
        return nested_handler;
    });
    auto pf = boost::get<PacketFunction>(p);
    fdd::leaf l = fdd::leaf{{ {oxm::field_set{}, pf} }};
    oxm::field_set fs{F<1>() == 1, F<2>() == 2, F<3>() == 3};

    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1, F<3>() == 3}, _)).Times(Between(1, 2));
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 1}, _)).Times(0);

    EXPECT_CALL(
        backend,
        install(
            oxm::field_set{F<1>() == 1, F<3>() == 3},
            match{oxm::field_set{F<2>() == 2}},
            _
        )
    ).Times(0);

    auto traces = retic::getTraces(l, fs);
    auto merged_trace = tracer::mergeTrace(traces, oxm::field_set{F<1>() == 1});
    trace_tree::Augmention augmenter( &root, &backend, oxm::field_set{F<1>() == 1}, 1, 1000);
    for (auto& n: merged_trace.values()) {
        boost::apply_visitor(augmenter, n);
    }
    auto next_fdd = augmenter.finish(merged_trace.result());
    oxm::field_set compiled_match = augmenter.match();
    EXPECT_EQ(next_fdd->value, true_next_fdd);
    oxm::field_set true_compiler_match = oxm::field_set{ F<1>() == 1, F<3>() == 3};
    EXPECT_EQ(compiled_match, true_compiler_match);
}

// TODO Test throw exceoption from methods


// TODO Test throw exceoption from methods


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/variant/apply_visitor.hpp>

#include "retic/tracer.hh"
#include "retic/trace_tree.hh"
#include "retic/trace_tree_translator.hh"
#include "retic/backend.hh"
#include "retic/fdd.hh"
#include "retic/policies.hh"
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
    trace_tree::Augmention augmenter(&root, backend, oxm::field_set{}, 10, 200);
    EXPECT_CALL(backend, install(oxm::field_set{}, match{oxm::field_set{F<1>() == 1}}, _));
    augmenter.finish(modify(F<1>() << 1));
}

TEST(OnLineTranslation, LoadMethod) {
    MockBackend backend;
    trace_tree::node root = trace_tree::unexplored{};
    tracer::trace_node n = tracer::load_node{F<1>() == 2};
    trace_tree::Augmention augmenter(&root, backend, oxm::field_set{}, 10, 200);
    boost::apply_visitor(augmenter, n);
    EXPECT_CALL(backend, install(oxm::field_set{F<1>() == 2}, match{oxm::field_set{F<1>() == 1}}, _));
    augmenter.finish(modify(F<1>() << 1));
}

TEST(OnLineTranslation, TestMethod) {
    MockBackend backend;
    uint16_t barrier_prio, positive_prio, negative_prio;
    trace_tree::node root = trace_tree::unexplored{};
    tracer::trace_node positive_n = tracer::test_node{F<1>() == 2, true};
    trace_tree::Augmention augmenter(&root, backend, oxm::field_set{}, 10, 200);
    EXPECT_CALL(backend, installBarrier(oxm::field_set{F<1>() == 2}, _))
        .WillOnce(SaveArg<1>(&barrier_prio));

    boost::apply_visitor(augmenter, positive_n);

    EXPECT_CALL(backend, install(oxm::field_set{F<1>() == 2}, match{oxm::field_set{F<1>() == 1}}, _))
        .WillOnce(SaveArg<2>(&positive_prio));
    augmenter.finish(modify(F<1>() << 1));

    EXPECT_CALL(backend, installBarrier(_, _)).Times(0);
    EXPECT_CALL(backend, install(oxm::field_set{}, match{oxm::field_set{F<2>() == 2}}, _))
        .WillOnce(SaveArg<2>(&negative_prio));

    tracer::trace_node negative_n = tracer::test_node({F<1>() == 2, false});
    trace_tree::Augmention negative_augmenter(&root, backend, oxm::field_set{}, 10, 200);
    boost::apply_visitor(negative_augmenter, negative_n);
    negative_augmenter.finish(modify(F<2>() << 2));

    EXPECT_LT(negative_prio, barrier_prio);
    EXPECT_LT(barrier_prio, positive_prio);

}

// TODO Test throw exceoption from methods


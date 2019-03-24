#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/variant/apply_visitor.hpp>

#include "retic/tracer.hh"
#include "retic/trace_tree.hh"
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

// TODO Test throw exceoption from methods


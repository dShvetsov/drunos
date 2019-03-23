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

TEST(TraceTreeTest, CreateTraceTree) {
    trace_tree::node root = trace_tree::unexplored{};
    trace_tree::Augmention augmenter(&root);
    oxm::field<> f = F<1>() == 1;
    tracer::trace_node n = tracer::load_node{f};
    boost::apply_visitor(augmenter, n);
    trace_tree::node true_value = 
        trace_tree::load_node{oxm::mask<>(f), {{f.value_bits(), trace_tree::unexplored{}}}};
    EXPECT_EQ(root, true_value);
}

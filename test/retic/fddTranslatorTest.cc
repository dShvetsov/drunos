#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.hh"

#include "retic/fdd.hh"
#include "retic/fdd_translator.hh"
#include "retic/trace_tree.hh"
#include "retic/trace_tree_translator.hh"
#include "retic/backend.hh"

#include "oxm/field_set.hh"

using namespace runos;
using namespace retic;
using namespace ::testing;

using match = std::vector<oxm::field_set>;

TEST(FddTranslation, TranslateEmpty) {
    MockBackend backend;
    fdd::Translator tranlator(backend);
    fdd::diagram d = fdd::leaf{};
    EXPECT_CALL(backend,
        install(
            oxm::field_set{},
            std::vector<oxm::field_set>{},
            _, _
        )
    );
    boost::apply_visitor(tranlator, d);
}

TEST(FddTranslation, TranslateOneAction) {
    MockBackend backend;
    EXPECT_CALL(backend,
        install(
            oxm::field_set{},
            match{oxm::field_set{}},
            _, _
        )
    );
    fdd::diagram d = fdd::leaf{{oxm::field_set{}}};
    fdd::Translator translator(backend);
    boost::apply_visitor(translator, d);
}

TEST(FddTranslation, TranslateSimpleNode) {
    MockBackend backend;
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{ oxm::field_set{F<2>() == 2} }},
        fdd::leaf{{ oxm::field_set{F<3>() == 3} }}
    };

    uint16_t prio_1, prio_2;
    EXPECT_CALL(backend, 
        install(
            oxm::field_set{F<1>() == 1},
            match{oxm::field_set{F<2>() == 2}},
            _, _
    )).WillOnce(SaveArg<2>(&prio_1));

    EXPECT_CALL(backend, install(oxm::field_set{}, match{oxm::field_set{F<3>() == 3}}, _, _))
        .WillOnce(SaveArg<2>(&prio_2));

    fdd::Translator translator{backend};
    boost::apply_visitor(translator, d);
    EXPECT_GE(prio_1, prio_2);
}

TEST(FddTranslation, EqualFieldsEqualPriorities) {
    MockBackend backend;
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{ oxm::field_set{F<10>() == 1}}},
        fdd::node {
            F<1>() == 2,
            fdd::leaf{{ oxm::field_set{F<20>() == 2} }},
            fdd::leaf{{ oxm::field_set{F<30>() == 3} }}
        },
    };

    uint16_t prio_1, prio_2, prio_3;
    EXPECT_CALL(backend,
        install(oxm::field_set{F<1>() == 1}, match{oxm::field_set{F<10>() == 1}}, _, _)
    ).WillOnce(SaveArg<2>(&prio_1));

    EXPECT_CALL(backend,
        install(oxm::field_set{F<1>() == 2}, match{oxm::field_set{F<20>() == 2}}, _, _)
    ).WillOnce(SaveArg<2>(&prio_2));

    EXPECT_CALL(backend, 
        install(oxm::field_set{}, match{oxm::field_set{F<30>() == 3}}, _, _)
    ).WillOnce(SaveArg<2>(&prio_3));

    fdd::Translator translator{backend};
    boost::apply_visitor(translator, d);
    EXPECT_EQ(prio_1, prio_2);
    EXPECT_GE(prio_2, prio_3);
}

TEST(FddTranslation, DiffFieldsDiffPriorities) {
    MockBackend backend;
    fdd::diagram d = fdd::node {
        F<1>() == 1,
        fdd::leaf{{ oxm::field_set{F<10>() == 1}}},
        fdd::node {
            F<2>() == 2,
            fdd::leaf{{ oxm::field_set{F<20>() == 2} }},
            fdd::leaf{{ oxm::field_set{F<30>() == 3} }}
        },
    };

    uint16_t prio_1, prio_2, prio_3;
    EXPECT_CALL(backend, 
        install(oxm::field_set{F<1>() == 1}, match{oxm::field_set{F<10>() == 1}}, _, _)
    ).WillOnce(SaveArg<2>(&prio_1));

    EXPECT_CALL(backend,
        install(oxm::field_set{F<2>() == 2}, match{oxm::field_set{F<20>() == 2}}, _, _)
    ).WillOnce(SaveArg<2>(&prio_2));

    EXPECT_CALL(backend,
        install(oxm::field_set{}, match{oxm::field_set{F<30>() == 3}}, _, _)
    ).WillOnce(SaveArg<2>(&prio_3));

    fdd::Translator translator{backend};
    boost::apply_visitor(translator, d);
    EXPECT_GE(prio_1, prio_2);
    EXPECT_GE(prio_2, prio_3);
}

TEST(FddTranslation, BarrierRule) {
    MockBackend backend;

    policy p = handler([](Packet& pkt){return stop();});
    auto pf = boost::get<PacketFunction>(p);
    fdd::diagram d = fdd::leaf{{ {oxm::field_set{}, pf}}};

    EXPECT_CALL(backend, install(_, _,_, _)).Times(0);
    EXPECT_CALL(backend, installBarrier(_, _)).Times(1);

    fdd::Translator translator{backend};
    boost::apply_visitor(translator, d);
}

TEST(FddTranslation, WithPreMatchAndPrios) {
    MockBackend backend;
    uint16_t prio;
    fdd::Translator tranlator(backend, oxm::field_set{F<1>() == 1}, 120, 400);
    fdd::diagram d = fdd::leaf{};
    EXPECT_CALL(backend,
        install(
            oxm::field_set{F<1>() == 1},
            std::vector<oxm::field_set>{},
            _, _
        )
    ).WillOnce(SaveArg<2>(&prio));
    boost::apply_visitor(tranlator, d);
    EXPECT_LE(120, prio) << "Priority must be greater than setted low priority";
    EXPECT_LE(prio, 400) << "Priority must be less than setted upper priority";
}

using secs = std::chrono::seconds;

TEST(FddTraverse, FlowSettings) {
    MockBackend backend;
    fdd::diagram d = fdd::leaf{{}, FlowSettings{secs(30), secs(40)}};
    fdd::Translator translator(backend);
    EXPECT_CALL(backend, install(_, _, _, FlowSettings{secs(30), secs(40)}));
    boost::apply_visitor(translator, d);
}

// TestTreeTranslation
//
TEST(TraceTreeTranslation, Unexplored) {
    MockBackend backend;
    trace_tree::node root = trace_tree::unexplored{};
    EXPECT_CALL(backend, install(_, _, _, _)).Times(0);
    EXPECT_CALL(backend, installBarrier(_, _)).Times(0);
    trace_tree::Translator translator{backend};
    boost::apply_visitor(translator, root);
}


// TODO: Test all methods of trace_tree::Translator


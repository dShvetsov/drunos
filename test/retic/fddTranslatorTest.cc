#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "retic/fdd.hh"
#include "retic/fdd_translator.hh"
#include "retic/backend.hh"

#include "oxm/field_set.hh"

using namespace runos;
using namespace retic;
using namespace ::testing;

template <size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true>
{ };

struct MockBackend: public Backend {
    MOCK_METHOD3(install, void(oxm::field_set, std::vector<oxm::field_set>, uint16_t));
    MOCK_METHOD2(installBarrier, void(oxm::field_set, uint16_t));
};

using match = std::vector<oxm::field_set>;

TEST(FddTranslation, TranslateEmpty) {
    MockBackend backend;
    fdd::Translator tranlator(backend);
    fdd::diagram d = fdd::leaf{};
    EXPECT_CALL(backend,
        install(
            oxm::field_set{},
            std::vector<oxm::field_set>{},
            _
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
            _
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
    EXPECT_CALL(backend, install(oxm::field_set{F<1>() == 1}, match{oxm::field_set{F<2>() == 2}}, _))
        .WillOnce(SaveArg<2>(&prio_1));

    EXPECT_CALL(backend, install(oxm::field_set{}, match{oxm::field_set{F<3>() == 3}}, _))
        .WillOnce(SaveArg<2>(&prio_2));

    fdd::Translator translator{backend};
    boost::apply_visitor(translator, d);
    EXPECT_GE(prio_1, prio_2);
}

TEST(FddTranslation, BarrierRule) {
    MockBackend backend;

    policy p = handler([](Packet& pkt){return stop();});
    auto pf = boost::get<PacketFunction>(p);
    fdd::diagram d = fdd::leaf{{ {oxm::field_set{}, pf}}};

    EXPECT_CALL(backend, install(_, _,_)).Times(0);
    EXPECT_CALL(backend, installBarrier(_, _)).Times(1);

    fdd::Translator translator{backend};
    boost::apply_visitor(translator, d);
}

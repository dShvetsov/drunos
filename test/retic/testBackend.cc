#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

#include "common.hh"

#include <vector>

#include "oxm/openflow_basic.hh"
#include "oxm/field_set.hh"

#include "retic/policies.hh"
#include "Retic.hh"
#include "OFDriver.hh"

using namespace runos;
using namespace retic;
using namespace ::testing;

using secs = std::chrono::seconds;

class MockDriver: public OFDriver {
public:
    MOCK_METHOD4(installRule, RulePtr(oxm::field_set, uint16_t, Actions, uint8_t));
    MOCK_METHOD2(installGroup, GroupPtr(GroupType, std::vector<Actions>));
    MOCK_METHOD3(packetOut, void(uint8_t* data, size_t data_len, Actions));
};


TEST(BackendTest, DropPacket) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;
    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver}
    };

    Actions actions = {};

    EXPECT_CALL(*mock_driver, 
        installRule(oxm::field_set{F<1>() == 1}, 10, actions, 2));

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{F<1>() == 1},
        {oxm::field_set{F<2>() == 2}},
        10,
        FlowSettings{}
    );
}

TEST(BackendTest, InstallSimpleRule) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;
    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver}
    };

    Actions actions = {.out_port = 101, .set_fields = oxm::field_set{F<2>() == 2}};

    EXPECT_CALL(*mock_driver,
        installRule(oxm::field_set{F<1>() == 1}, 10, actions, 2));

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{F<1>() == 1},
        {oxm::field_set{F<2>() == 2, oxm::out_port() == 101}},
        10,
        FlowSettings{}
    );
}

TEST(BackendTest, NoActions) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;
    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver}
    };

    Actions actions = {};

    EXPECT_CALL(*mock_driver,
        installRule(oxm::field_set{F<1>() == 1}, 10, actions, 2));

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{F<1>() == 1},
        {},
        10,
        FlowSettings{}
    );
}


TEST(BackendTest, MultiActions) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;
    Actions a1 = {.out_port = 3, .set_fields = oxm::field_set{F<3>() == 3}};
    Actions a2 = {.out_port = 2, .set_fields = oxm::field_set{F<2>() == 2}};

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver}
    };

    struct FakeGroup: public Group {
        uint32_t id() const override { return 634; }
    };

    GroupPtr group = std::make_shared<FakeGroup>();

    EXPECT_CALL(*mock_driver,
        installGroup(GroupType::All, UnorderedElementsAre(a1, a2))
    ).WillOnce(Return(group));

    Actions to_group = {.group_id = 634};

    EXPECT_CALL(*mock_driver, 
        installRule(oxm::field_set{F<1>() == 1}, 10, to_group, 2));

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{F<1>() == 1},
        {
            oxm::field_set{F<2>() == 2, oxm::out_port() == 2},
            oxm::field_set{F<3>() == 3, oxm::out_port() == 3}
        },
        10,
        FlowSettings{}
    );
}

TEST(BackendTest, TwoSwitch) {
    auto mock_driver1 = std::make_shared<MockDriver>();
    OFDriverPtr driver1 = mock_driver1;

    auto mock_driver2 = std::make_shared<MockDriver>();
    OFDriverPtr driver2 = mock_driver2;

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver1},
        {2, driver2}
    };

    Actions acts = {};

    EXPECT_CALL(*mock_driver1,
        installRule(oxm::field_set{}, 10, acts, 2));

    EXPECT_CALL(*mock_driver2,
        installRule(oxm::field_set{}, 10, acts, 2));

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{},
        {},
        10,
        FlowSettings{}
    );
}

TEST(BackendTest, TwoSwitchOneRule) {
    auto mock_driver1 = std::make_shared<MockDriver>();
    OFDriverPtr driver1 = mock_driver1;

    auto mock_driver2 = std::make_shared<MockDriver>();
    OFDriverPtr driver2 = mock_driver2;

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver1},
        {2, driver2}
    };

    Actions acts = {};

    EXPECT_CALL(*mock_driver1,
        installRule(_, _, _, _)).Times(0);

    EXPECT_CALL(*mock_driver2,
        installRule(oxm::field_set{}, 10, acts, 2));

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{oxm::switch_id() == 2},
        {},
        10,
        FlowSettings{}
    );
}

TEST(BackendTest, NoSwitchInAction) {
    auto mock_driver1 = std::make_shared<MockDriver>();
    OFDriverPtr driver1 = mock_driver1;

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver1}
    };

    Actions acts = {};

    EXPECT_CALL(*mock_driver1,
        installRule(_, _, _, _)).Times(0);

    Of13Backend backend(drivers, 2);
    backend.install(
        oxm::field_set{oxm::switch_id() == 2},
        {},
        10,
        FlowSettings{}
    );
}

TEST(BackendTest, TwoSwitchOneBarrierRule) {
    auto mock_driver1 = std::make_shared<MockDriver>();
    OFDriverPtr driver1 = mock_driver1;

    auto mock_driver2 = std::make_shared<MockDriver>();
    OFDriverPtr driver2 = mock_driver2;

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver1},
        {2, driver2}
    };

    Actions acts = {.out_port = ports::to_controller};

    EXPECT_CALL(*mock_driver1,
        installRule(_, _, _, _)).Times(0);

    EXPECT_CALL(*mock_driver2,
        installRule(oxm::field_set{}, 10, acts, 2));

    Of13Backend backend(drivers, 2);
    backend.installBarrier(
        oxm::field_set{oxm::switch_id() == 2},
        10
    );
}

TEST(BackendTest, BarrierRule) {
    auto mock_driver1 = std::make_shared<MockDriver>();
    OFDriverPtr driver1 = mock_driver1;

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver1}
    };

    Actions acts = {.out_port = ports::to_controller};

    EXPECT_CALL(*mock_driver1,
        installRule(oxm::field_set{}, 10, acts, 2));

    Of13Backend backend(drivers, 2);
    backend.installBarrier(
        oxm::field_set{},
        10
    );
}

TEST(BackendTest, BarrierRuleNoSwitch) {
    auto mock_driver1 = std::make_shared<MockDriver>();
    OFDriverPtr driver1 = mock_driver1;

    std::unordered_map<uint64_t, OFDriverPtr> drivers {
        {1, driver1}
    };

    Actions acts = {.out_port = ports::to_controller};

    EXPECT_CALL(*mock_driver1,
        installRule(_, _, _, _)).Times(0);

    Of13Backend backend(drivers, 2);
    backend.installBarrier(
        oxm::field_set{oxm::switch_id() == 2},
        10
    );
}

TEST(BackendTest, TestPacketOuts) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;

    std::vector<oxm::field_set> actions = {
        oxm::field_set{oxm::out_port() == 1},
        oxm::field_set{F<1>() == 1, oxm::out_port() == 2},
        oxm::field_set{F<2>() == 2}
    };

    Of13Backend backend({{1, driver}}, 1);

    uint8_t data[16];
    size_t data_len = 16;

    EXPECT_CALL(*mock_driver, packetOut(data, data_len, Actions{.out_port = 1}));
    EXPECT_CALL(
        *mock_driver,
        packetOut(
            data, data_len,
            Actions{.out_port = 2, .set_fields = oxm::field_set{F<1>() == 1}}
        )
    );

    backend.packetOuts(data, data_len, actions, 1);
}

TEST(BackendTest, FlowSettings) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;

    Of13Backend backend({{1, driver}}, 1);
    std::vector<oxm::field_set> actions = {
        oxm::field_set{oxm::out_port() == 1}
    };

    EXPECT_CALL(*mock_driver,
        installRule(
            _, _, Actions{.out_port = 1, .idle_timeout = 10, .hard_timeout = 20}, _
        )
    ).Times(1);

    backend.install(
        oxm::field_set{}, actions, 10,
        retic::FlowSettings{.idle_timeout = secs(10), .hard_timeout = secs(20)}
    );
}

TEST(BackendTest, FlowSettingsInfinitySettings) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;

    Of13Backend backend({{1, driver}}, 1);
    std::vector<oxm::field_set> actions = {
        oxm::field_set{oxm::out_port() == 1}
    };

    EXPECT_CALL(*mock_driver,
        installRule(
            _, _, Actions{.out_port = 1, .idle_timeout = 0, .hard_timeout = 0}, _
        )
    ).Times(1);

    backend.install(
        oxm::field_set{}, actions, 10,
        retic::FlowSettings{.idle_timeout = duration::max(), .hard_timeout = duration::max()}
    );
}

TEST(BackendTest, FlowSettingsNoInstallRule) {
    auto mock_driver = std::make_shared<MockDriver>();
    OFDriverPtr driver = mock_driver;

    Of13Backend backend({{1, driver}}, 1);
    std::vector<oxm::field_set> actions = {
        oxm::field_set{oxm::out_port() == 1}
    };

    EXPECT_CALL(*mock_driver, installRule(_, _, _, _)).Times(0);

    backend.install(
        oxm::field_set{}, actions, 10,
        retic::FlowSettings{.idle_timeout = duration::zero(), .hard_timeout = duration::zero()}
    );
}

add_subdirectory(types)
add_subdirectory(maple)
add_subdirectory(retic)

set(TESTABLE_SOURCES
    Application.cc
    Loader.cc
    OFMsgUnion.cc
    OFTransaction.cc
    FluidOXMAdapter.cc
    SwitchConnection.cc
    PacketParser.cc
    Controller.cc
    Retic.cc
    OFDriver.hh
    OFDriver.cc
    json11.cpp
)

set(SOURCES
    # Core
    Application.cc
    Loader.cc
    Decision.cc
    Flow.cc
    OFMsgUnion.cc
    OFTransaction.cc
    FluidOXMAdapter.cc
    SwitchConnection.cc
    PacketParser.cc
    Controller.cc
    Switch.cc
    LinkDiscovery.cc
    Topology.cc
    STP.cc
    Maple.cc
    CommandLine.cc
    Retic.cc
    OFDriver.hh
    OFDriver.cc
    # Apps
    SimpleLearningSwitch.cc
    LearningSwitch.cc
    CBench.cc
    Stats.cc
    DropAll.cc
    NoMapleRules.cc
#ArpHandler.cc
    StaticFlowPusher.hh
    StaticFlowPusher.cc
    RestMultipart.cc
    RestFlowMod.cc
    RestStringProcessing.cc
    # Loader
    # REST
    RestListener.cc
    Event.cc
    AppObject.cc
    HostManager.cc
    WebUIManager.cc
    FlowManager.cc
    # Json
    json11.cpp
    SwitchCli.cc
    ReticCli.cc
    # funcs test
    TestApps.cc
)

add_definitions(
    -DBOOST_ENABLE_ASSERT_HANDLER
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR} # for Qt-generated moc files
    ${CMAKE_SOURCE_DIR}/third_party/webserver
)

add_library(runos_base STATIC ${TESTABLE_SOURCES})

target_link_libraries(runos_base
        Qt5::Core)

add_executable(runos Main.cc ${SOURCES})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

target_link_libraries(runos
# runos_base
    runos_types
    runos_maple
    runos_retic
    tcmalloc
    Qt5::Core
    fluid_base
    libfluid_msg.a
    edit
    fmt
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_COROUTINE_LIBRARY}
    ${GLOG_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_LIBRARIES}
    pthread
)

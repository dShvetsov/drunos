
add_executable(runReticTest
        runReticTest.cc
        reticTest.cc
        reticPacketParse.cc
        fddTest.cc
        fddTranslatorTest.cc
        testBackend.cc
        testTracer.cc
        testTraceTree.cc
)

target_link_libraries(runReticTest
    ${TEST_LINK_LIBRARIES}
    runos_base
    runos_types
    runos_maple
    runos_retic
    libfluid_msg.a
    fluid_base
)

add_test(NAME runReticTest COMMAND runReticTest)

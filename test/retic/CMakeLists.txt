
add_executable(runReticTest
        runReticTest.cc
        reticTest.cc
        reticPacketParse.cc
        fddTest.cc
)

target_link_libraries(runReticTest
    ${TEST_LINK_LIBRARIES}
    libfluid_msg.a
    runos_base
    runos_types
    runos_retic
)

add_test(NAME runReticTest COMMAND runReticTest)

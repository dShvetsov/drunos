
add_executable(runReticTest
        runReticTest.cc
        reticTest.cc
        reticPacketParse.cc
)

target_link_libraries(runReticTest
    ${TEST_LINK_LIBRARIES}
    libfluid_msg.a
    runos_types
)

add_test(NAME runReticTest COMMAND runReticTest)

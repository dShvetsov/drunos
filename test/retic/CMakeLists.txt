
add_executable(runReticTest
        runReticTest.cc
        reticTest.cc
        )
target_link_libraries(runReticTest
    ${TEST_LINK_LIBRARIES}
    runos_types
    )
add_test(NAME runReticTest COMMAND runReticTest)

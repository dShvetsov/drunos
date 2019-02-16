set(CMAKE_AUTOMOC OFF)

set(SOURCES
    fdd.hh
    fdd.cc
)

add_library(runos_retic STATIC ${SOURCES})

target_link_libraries(runos_retic
       ${GLOG_LIBRARIES}
       ${Boost_LIBRARIES}
)

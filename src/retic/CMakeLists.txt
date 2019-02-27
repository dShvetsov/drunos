set(CMAKE_AUTOMOC OFF)

set(SOURCES
    fdd.hh
    applier.hh
    dumper.hh
    fdd_translator.hh
    fdd_translator.cc
    policies.hh
    fdd_compiler.cc
    fdd_compiler.hh
)

add_library(runos_retic STATIC ${SOURCES})

target_link_libraries(runos_retic
       ${GLOG_LIBRARIES}
       ${Boost_LIBRARIES}
)

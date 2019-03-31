set(CMAKE_AUTOMOC OFF)

set(SOURCES
    fdd.hh
    applier.hh
    dumper.hh
    fdd_translator.hh
    fdd_translator.cc
    policies.hh
    policies.cc
    fdd_compiler.cc
    fdd_compiler.hh
    traverse_fdd.cc
    traverse_fdd.hh
    trace_tree.hh
    trace_tree.cc
    tracer.hh
    tracer.cc
    leaf_applier.cc
    leaf_applier.hh
    trace_tree_translator.cc
    trace_tree_translator.hh
    traverse_trace_tree.cc
    traverse_trace_tree.hh
)

add_library(runos_retic STATIC ${SOURCES})

target_link_libraries(runos_retic
       runos_maple
       ${GLOG_LIBRARIES}
       ${Boost_LIBRARIES}
)

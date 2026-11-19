add_library(prometheus_topology STATIC
    src/index_resource.cpp
    src/operations.cpp
    src/schema.cpp
    src/slot_pool.cpp
)
target_include_directories(prometheus_topology PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(prometheus_topology PUBLIC prometheus_common prometheus_geometry)

if(BUILD_TESTING)
    add_executable(topology_tests
        src/test_schema.cpp
        src/test_schema_json.cpp
        src/test_index_resource.cpp
        src/test_slot_pool.cpp
    )
    target_link_libraries(topology_tests PRIVATE
        prometheus_topology
        GTest::gtest_main
    )
    gtest_discover_tests(topology_tests)
endif()

add_library(prometheus_topology STATIC
    src/operations.cpp
    src/schema.cpp
)
target_include_directories(prometheus_topology PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(prometheus_topology PUBLIC prometheus_common prometheus_containers prometheus_geometry)

if(BUILD_TESTING)
    add_executable(topology_tests
        src/test_schema.cpp
        src/test_schema_json.cpp
    )
    target_link_libraries(topology_tests PRIVATE
        prometheus_topology
        GTest::gtest_main
    )
    gtest_discover_tests(topology_tests)
endif()

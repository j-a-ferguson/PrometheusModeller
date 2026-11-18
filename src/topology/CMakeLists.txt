add_library(prometheus_topology STATIC
    src/schema.cpp
    src/utils.cpp
)
target_include_directories(prometheus_topology
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(prometheus_topology PUBLIC armadillo prometheus_geometry)

if(BUILD_TESTING)
    add_executable(topology_tests
        src/test_schema.cpp
        src/test_utils.cpp
    )
    target_include_directories(topology_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(topology_tests PRIVATE
        prometheus_topology
        GTest::gtest_main
    )
    gtest_discover_tests(topology_tests)
endif()

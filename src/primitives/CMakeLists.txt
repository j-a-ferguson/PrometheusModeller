add_library(prometheus_primitives STATIC src/primitives.cpp)

target_include_directories(prometheus_primitives PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(prometheus_primitives PUBLIC prometheus_common prometheus_geometry prometheus_topology)

if(BUILD_TESTING)
    add_executable(primitives_tests
        src/test_primitives.cpp
    )
    target_link_libraries(primitives_tests PRIVATE
        prometheus_primitives
        GTest::gtest_main
    )
    gtest_discover_tests(primitives_tests)
endif()

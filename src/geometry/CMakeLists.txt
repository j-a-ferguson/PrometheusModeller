add_library(prometheus_geometry STATIC
    src/curves.cpp
    src/surfaces.cpp
    src/utils.cpp
)
target_include_directories(prometheus_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(prometheus_geometry PUBLIC prometheus_common)

if(BUILD_TESTING)
    add_executable(geometry_tests
        src/test_curves.cpp
        src/test_utils.cpp
    )
    target_link_libraries(geometry_tests PRIVATE
        prometheus_geometry
        GTest::gtest_main
    )
    gtest_discover_tests(geometry_tests)
endif()

add_library(prometheus_containers INTERFACE)
target_include_directories(prometheus_containers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(prometheus_containers INTERFACE cxx_std_23)
target_link_libraries(prometheus_containers INTERFACE prometheus_common)

if(BUILD_TESTING)
    add_executable(containers_tests
        src/test_inplace_vector.cpp
        src/test_slot_pool.cpp
    )
    target_link_libraries(containers_tests PRIVATE prometheus_containers GTest::gtest_main)
    gtest_discover_tests(containers_tests)
endif()

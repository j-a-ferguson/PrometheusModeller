add_library(prometheus_validate STATIC 
    src/validate.cpp 
    src/validate_general.cpp 
    src/validate_manifold_sheet.cpp 
    src/validate_manifold_solid.cpp 
    src/validate_manifold_wire.cpp)


target_include_directories(prometheus_validate 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_link_libraries(prometheus_validate PUBLIC prometheus_common prometheus_geometry prometheus_topology)


if(BUILD_TESTING)
    add_executable(validate_tests
        src/test_validate.cpp
    )
    target_link_libraries(validate_tests PRIVATE
        prometheus_validate
        GTest::gtest_main
    )
    gtest_discover_tests(validate_tests)
endif()
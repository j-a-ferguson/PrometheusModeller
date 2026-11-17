cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain")
endif()

project(PrometheusModeller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Armadillo CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Sibling project; its tests/examples are off automatically when not top-level
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../PrometheusOptimize
                 ${CMAKE_CURRENT_BINARY_DIR}/PrometheusOptimize)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
    add_executable(prometheus_modeller_tests tests/modeller_test.cpp)
    target_link_libraries(prometheus_modeller_tests PRIVATE
        prometheus_modeller
        GTest::gtest_main
    )
    gtest_discover_tests(prometheus_modeller_tests)
endif()

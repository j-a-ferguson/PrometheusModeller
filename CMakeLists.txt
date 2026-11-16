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

add_library(prometheus_modeller STATIC src/modeller.cpp)
target_include_directories(prometheus_modeller
    PUBLIC include
    PRIVATE src
)
target_link_libraries(prometheus_modeller PUBLIC
    armadillo
    nlohmann_json::nlohmann_json
)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(prometheus_modeller_tests tests/modeller_test.cpp)
    target_link_libraries(prometheus_modeller_tests PRIVATE
        prometheus_modeller
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(prometheus_modeller_tests)
endif()

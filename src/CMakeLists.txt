add_library(prometheus_api INTERFACE)
target_include_directories(prometheus_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/api)
target_compile_features(prometheus_api INTERFACE cxx_std_23)
target_link_libraries(prometheus_api INTERFACE armadillo)

add_subdirectory(common)
add_subdirectory(containers)
add_subdirectory(geometry)
add_subdirectory(topology)
add_subdirectory(primitives)
add_subdirectory(validate)

add_library(prometheus_modeller STATIC modeller.cpp)
target_link_libraries(prometheus_modeller
    PUBLIC prometheus_api
    PRIVATE prometheus_geometry prometheus_topology
)

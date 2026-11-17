add_subdirectory(geometry)
add_subdirectory(topology)

add_library(prometheus_modeller STATIC
    modeller.cpp
)
target_include_directories(prometheus_modeller
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(prometheus_modeller PUBLIC
    prometheus_geometry
    prometheus_topology
    nlohmann_json::nlohmann_json
)

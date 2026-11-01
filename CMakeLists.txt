cmake_minimum_required(VERSION 3.20)
project(MayhemBuildingForge VERSION 0.2.0 LANGUAGES CXX)
option(MBF_BUILD_TESTS "Build tests" ON)
option(MBF_BUILD_EXAMPLES "Build example applications" ON)
add_library(mbf_core
    core/src/floor_plan.cpp
    core/src/generator.cpp
    core/src/geometry2d.cpp
    core/src/polygon.cpp
    core/src/polygon_intersection.cpp
    core/src/polygon_region.cpp
    core/src/polygon_offset.cpp
    core/src/residential_layout.cpp
    core/src/multi_floor.cpp
    core/src/building_brief.cpp
    core/src/topology.cpp
    core/src/topology_svg.cpp
    core/src/spatial_quality.cpp
)
add_library(MBF::Core ALIAS mbf_core)
target_include_directories(mbf_core PUBLIC core/include)
target_compile_features(mbf_core PUBLIC cxx_std_20)
set_target_properties(mbf_core PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(mbf_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(mbf_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
if(MBF_BUILD_TESTS)
    enable_testing()
    add_executable(mbf_tests tests/test_main.cpp)
    target_link_libraries(mbf_tests PRIVATE MBF::Core)
    add_test(NAME mbf_core_tests COMMAND mbf_tests)
endif()
if(MBF_BUILD_EXAMPLES)
    add_executable(mbf_generate_sample examples/generate_sample.cpp)
    target_link_libraries(mbf_generate_sample PRIVATE MBF::Core)
    add_executable(mbf_polygon_demo examples/polygon_demo.cpp)
    target_link_libraries(mbf_polygon_demo PRIVATE MBF::Core)
    add_executable(mbf_house_layout_demo examples/house_layout_demo.cpp)
    target_link_libraries(mbf_house_layout_demo PRIVATE MBF::Core)
    add_executable(mbf_build_house examples/mbf_build_house.cpp)
    target_link_libraries(mbf_build_house PRIVATE MBF::Core)
    add_executable(mbf_topology_preview examples/mbf_topology_preview.cpp)
    target_link_libraries(mbf_topology_preview PRIVATE MBF::Core)
    add_executable(mbf_generate_topology_house examples/mbf_generate_topology_house.cpp)
    target_link_libraries(mbf_generate_topology_house PRIVATE MBF::Core)
    if(MBF_BUILD_TESTS)
        add_test(NAME mbf_topology_preview_smoke
            COMMAND mbf_topology_preview --bedrooms 2
                    --output ${CMAKE_CURRENT_BINARY_DIR}/topology_smoke.svg)
        add_test(NAME mbf_topology_house_smoke
            COMMAND mbf_generate_topology_house --width 18 --depth 12
                    --bedrooms 2 --output-prefix ${CMAKE_CURRENT_BINARY_DIR}/topology_house_smoke)
        add_test(NAME mbf_topology_house_rejects_unimplemented_multifloor
            COMMAND mbf_generate_topology_house --floors 2)
        set_tests_properties(mbf_topology_house_rejects_unimplemented_multifloor
            PROPERTIES WILL_FAIL TRUE)
    endif()
endif()

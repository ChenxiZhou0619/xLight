cmake_minimum_required(VERSION 3.12)

project(XLight LANGUAGES CXX)

set(XLIGHT_CORE_MESH_DIR ${PROJECT_SOURCE_DIR}/src/core/mesh)
set(XLIGHT_TEST_DIR ${PROJECT_SOURCE_DIR}/src/test)
set(XLIGHT_CORE_RENDER_DIR ${PROJECT_SOURCE_DIR}/src/core/render-core)
set(XLIGHT_CORE_ACCEL_DIR ${PROJECT_SOURCE_DIR}/src/core/accelerate)
set(XLIGHT_RENDER_INTEGRATOR_DIR ${PROJECT_SOURCE_DIR}/src/render/integrator)
set(XLIGHT_RENDER_SAMPLER_DIR ${PROJECT_SOURCE_DIR}/src/render/sampler)

add_executable(test
    # test
    ${XLIGHT_TEST_DIR}/test.cpp

    # core/accelerate
    ${XLIGHT_CORE_ACCEL_DIR}/accel.cpp
    ${XLIGHT_CORE_ACCEL_DIR}/octree.cpp

    # core/mesh
    ${XLIGHT_CORE_MESH_DIR}/meshLoader.cpp
    ${XLIGHT_CORE_MESH_DIR}/meshSet.cpp
    ${XLIGHT_CORE_MESH_DIR}/mesh.cpp

    # core/render-core
    ${XLIGHT_CORE_RENDER_DIR}/scene.cpp

    # render/integrator
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/simple.cpp

    # render/sampelr
    ${XLIGHT_RENDER_SAMPLER_DIR}/independent.cpp
)

target_include_directories(test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/dependencies
)

target_link_directories(test PRIVATE
    /usr/local/lib
)

target_link_libraries(test
    assimp tbb
)
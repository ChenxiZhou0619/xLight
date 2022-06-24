cmake_minimum_required(VERSION 3.12)

project(XLight LANGUAGES CXX)

set(XLIGHT_TEST_DIR ${PROJECT_SOURCE_DIR}/src/test)

set(XLIGHT_CORE_MESH_DIR ${PROJECT_SOURCE_DIR}/src/core/mesh)
set(XLIGHT_CORE_RENDER_DIR ${PROJECT_SOURCE_DIR}/src/core/render-core)
set(XLIGHT_CORE_ACCEL_DIR ${PROJECT_SOURCE_DIR}/src/core/accelerate)
set(XLIGHT_CORE_UTIL_DIR ${PROJECT_SOURCE_DIR}/src/core/utils)
set(XLIGHT_CORE_FILE_DIR ${PROJECT_SOURCE_DIR}/src/core/file)

set(XLIGHT_RENDER_INTEGRATOR_DIR ${PROJECT_SOURCE_DIR}/src/render/integrator)
set(XLIGHT_RENDER_SAMPLER_DIR ${PROJECT_SOURCE_DIR}/src/render/sampler)
set(XLIGHT_RENDER_CAMERA_DIR ${PROJECT_SOURCE_DIR}/src/render/camera)
set(XLIGHT_RENDER_TEXTURE_DIR ${PROJECT_SOURCE_DIR}/src/render/texture)
set(XLIGHT_RENDER_BSDF_DIR ${PROJECT_SOURCE_DIR}/src/render/bsdf)
set(XLIGHT_RENDER_EMITTER_DIR ${PROJECT_SOURCE_DIR}/src/render/emitter)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test
    # test
    ${XLIGHT_TEST_DIR}/test.cpp

    # core/accelerate
    ${XLIGHT_CORE_ACCEL_DIR}/accel.cpp
    ${XLIGHT_CORE_ACCEL_DIR}/octree.cpp

    # core/file
    ${XLIGHT_CORE_FILE_DIR}/image.cpp

    # core/mesh
    ${XLIGHT_CORE_MESH_DIR}/meshLoader.cpp
    ${XLIGHT_CORE_MESH_DIR}/meshSet.cpp
    ${XLIGHT_CORE_MESH_DIR}/mesh.cpp

    # core/render-core
    ${XLIGHT_CORE_RENDER_DIR}/scene.cpp
    ${XLIGHT_CORE_RENDER_DIR}/texture.cpp
    ${XLIGHT_CORE_RENDER_DIR}/sampler.cpp

    # core/utils
    ${XLIGHT_CORE_UTIL_DIR}/factory.cpp
    ${XLIGHT_CORE_UTIL_DIR}/taskparser.cpp

    # render/integrator
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/simple.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/whitted.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/path.cpp

    # render/sampler
    ${XLIGHT_RENDER_SAMPLER_DIR}/independent.cpp
    ${XLIGHT_RENDER_SAMPLER_DIR}/stratified.cpp

    # render/camera
    ${XLIGHT_RENDER_CAMERA_DIR}/perspective.cpp
    ${XLIGHT_RENDER_CAMERA_DIR}/thinlens.cpp

    # render/texture
    ${XLIGHT_RENDER_TEXTURE_DIR}/constant.cpp
    ${XLIGHT_RENDER_TEXTURE_DIR}/bitmap.cpp

    # render/bsdf
    ${XLIGHT_RENDER_BSDF_DIR}/diffuse.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/mirror.cpp

    # render/emitter
    ${XLIGHT_RENDER_EMITTER_DIR}/area.cpp
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
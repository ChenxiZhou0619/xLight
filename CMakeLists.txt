cmake_minimum_required(VERSION 3.12)

project(XLight LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/target/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/target/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/target/bin)

set(XLIGHT_TEST_DIR ${PROJECT_SOURCE_DIR}/src/test)

set(XLIGHT_CORE_RENDER_DIR ${PROJECT_SOURCE_DIR}/src/core/render-core)
set(XLIGHT_CORE_ACCEL_DIR ${PROJECT_SOURCE_DIR}/src/core/accelerate)
set(XLIGHT_CORE_UTIL_DIR ${PROJECT_SOURCE_DIR}/src/core/utils)
set(XLIGHT_CORE_FILE_DIR ${PROJECT_SOURCE_DIR}/src/core/file)
set(XLIGHT_CORE_SHAPE_DIR ${PROJECT_SOURCE_DIR}/src/core/shape)
set(XLIGHT_CORE_SCENE_DIR ${PROJECT_SOURCE_DIR}/src/core/scene)
set(XLIGHT_CORE_TASK_DIR ${PROJECT_SOURCE_DIR}/src/core/task)

set(XLIGHT_RENDER_INTEGRATOR_DIR ${PROJECT_SOURCE_DIR}/src/render/integrator)
set(XLIGHT_RENDER_SAMPLER_DIR ${PROJECT_SOURCE_DIR}/src/render/sampler)
set(XLIGHT_RENDER_CAMERA_DIR ${PROJECT_SOURCE_DIR}/src/render/camera)
set(XLIGHT_RENDER_TEXTURE_DIR ${PROJECT_SOURCE_DIR}/src/render/texture)
set(XLIGHT_RENDER_BSDF_DIR ${PROJECT_SOURCE_DIR}/src/render/bsdf)
set(XLIGHT_RENDER_EMITTER_DIR ${PROJECT_SOURCE_DIR}/src/render/emitter)
set(XLIGHT_RENDER_MEDIUM_DIR ${PROJECT_SOURCE_DIR}/src/render/medium)
set(XLIGHT_RENDER_FILTER_DIR ${PROJECT_SOURCE_DIR}/src/render/filter)
set(XLIGHT_RENDER_BSSRDF_DIR ${PROJECT_SOURCE_DIR}/src/render/bssrdf)

set(XLIGHT_GUI_DIR ${PROJECT_SOURCE_DIR}/src/gui)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(./dependencies)

add_executable(
    demo    ${XLIGHT_TEST_DIR}/demo.cpp

    ${XLIGHT_CORE_UTIL_DIR}/factory.cpp
    ${XLIGHT_CORE_UTIL_DIR}/configurable.cpp
    
    ${XLIGHT_CORE_SHAPE_DIR}/mesh.cpp
    ${XLIGHT_CORE_SHAPE_DIR}/gridmedium.cpp
    
    ${XLIGHT_CORE_SCENE_DIR}/scene.cpp
    
    ${XLIGHT_CORE_TASK_DIR}/task.cpp

    # core/render-core
    ${XLIGHT_CORE_RENDER_DIR}/bsdf.cpp
    ${XLIGHT_CORE_RENDER_DIR}/medium.cpp
    ${XLIGHT_CORE_RENDER_DIR}/texture.cpp
    ${XLIGHT_CORE_RENDER_DIR}/sampler.cpp
    ${XLIGHT_CORE_RENDER_DIR}/info.cpp

    # core/file
    ${XLIGHT_CORE_FILE_DIR}/figure.cpp

    
    # render/integrator
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/integrator.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/lighttracer.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/deep.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/normal.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/pathtracer.cpp
#    ${XLIGHT_RENDER_INTEGRATOR_DIR}/volpathtracer.cpp
    ${XLIGHT_RENDER_INTEGRATOR_DIR}/bidirpathtracer.cpp

    # render/sampler
    ${XLIGHT_RENDER_SAMPLER_DIR}/independent.cpp
    ${XLIGHT_RENDER_SAMPLER_DIR}/stratified.cpp

    # render/camera
    ${XLIGHT_RENDER_CAMERA_DIR}/perspective.cpp
    ${XLIGHT_RENDER_CAMERA_DIR}/thinlens.cpp

    # render/texture
    ${XLIGHT_RENDER_TEXTURE_DIR}/constant.cpp
    ${XLIGHT_RENDER_TEXTURE_DIR}/bitmap.cpp
    ${XLIGHT_RENDER_TEXTURE_DIR}/checkboard.cpp
    ${XLIGHT_RENDER_TEXTURE_DIR}/mipmap.cpp
    # render/bsdf
    ${XLIGHT_RENDER_BSDF_DIR}/diffuse.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/mirror.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/dielectric.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/microfacet.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/roughdielectric.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/plastic.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/empty.cpp
    ${XLIGHT_RENDER_BSDF_DIR}/disney.cpp

    # render/bssrdf
    ${XLIGHT_RENDER_BSSRDF_DIR}/seperate.cpp

    ${XLIGHT_RENDER_FILTER_DIR}/filter.cpp
    # render/emitter
    ${XLIGHT_RENDER_EMITTER_DIR}/area.cpp
    ${XLIGHT_RENDER_EMITTER_DIR}/environment.cpp
    ${XLIGHT_RENDER_EMITTER_DIR}/spot.cpp
    # render/medium
    ${XLIGHT_RENDER_MEDIUM_DIR}/beerslaw.cpp
    ${XLIGHT_RENDER_MEDIUM_DIR}/homogeneous.cpp
    ${XLIGHT_RENDER_MEDIUM_DIR}/hetergeneous.cpp
    ${XLIGHT_RENDER_MEDIUM_DIR}/phase.cpp
)

target_include_directories(demo PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/dependencies 
)

target_link_directories(demo PRIVATE
    ${PROJECT_SOURCE_DIR}/target/lib
    /usr/local/lib
)

target_link_libraries(demo
    assimp embree3 tbb openvdb spdlog profiler unwind tinyexr
)

#add_executable(
#    test ${XLIGHT_TEST_DIR}/test.cpp
#
#    ${XLIGHT_CORE_FILE_DIR}/figure.cpp
#)
#
#target_include_directories(test PRIVATE
#    ${PROJECT_SOURCE_DIR}/src
#    ${PROJECT_SOURCE_DIR}/dependencies 
#)
#
#target_link_libraries(demo
#    fmt
#)
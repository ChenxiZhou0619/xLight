cmake_minimum_required(VERSION 3.12)

project(XLight LANGUAGES CXX)

set(XLIGHT_CORE_MESH_DIR ${PROJECT_SOURCE_DIR}/src/core/mesh)
set(XLIGHT_TEST_DIR ${PROJECT_SOURCE_DIR}/src/test)

add_executable(test
    # test
    ${XLIGHT_TEST_DIR}/test.cpp

    # core/mesh
    ${XLIGHT_CORE_MESH_DIR}/meshLoader.cpp
)

target_include_directories(test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_directories(test PRIVATE
    /usr/local/lib
)

target_link_libraries(test
    assimp
)
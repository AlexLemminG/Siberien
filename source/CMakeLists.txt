cmake_minimum_required(VERSION 3.1)

# set the project name
project(SiberienEngine)

set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL")
set(BUILD_BGFX_CMD "${CMAKE_CURRENT_SOURCE_DIR}/../tools/genie.exe")
MESSAGE("COMMAND: ${BUILD_BGFX_CMD}")
execute_process(COMMAND ${BUILD_BGFX_CMD} --with-examples --with-sdl vs2019
   RESULT_VARIABLE CMD_ERROR
   OUTPUT_FILE CMD_OUTPUT
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/bgfx)
MESSAGE( STATUS "BGFX_BUILD_ERROR:" ${CMD_ERROR})
MESSAGE( STATUS "BGFX_BUILD_OUTPUT:" ${CMD_OUTPUT})

add_definitions(-DUNICODE)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(BUILD_SHARED_LIBS False)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/assimp)

add_subdirectory(libs/optick)

add_subdirectory(libs/SDL)

add_subdirectory(libs/rapidyaml)

add_subdirectory(libs/yaml-cpp)

add_subdirectory(libs/lz4/lib)

set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/RecastDetour)

add_subdirectory(libs/physfs)

set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/bullet)

# add_subdirectory(libs/bgfx)

# add_subdirectory(libs/bgfx/.build/projects/cmake/bx)

# add_subdirectory(libs/bgfx/.build/projects/cmake/bimg)

add_subdirectory(libs/compressonator/cmp_compressonatorlib)

add_subdirectory(Engine/SEngine)
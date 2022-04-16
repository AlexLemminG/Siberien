cmake_minimum_required(VERSION 3.10)

# set the project name
project(SiberienEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/assimp)

add_subdirectory(libs/optick)

add_subdirectory(libs/SDL)

add_subdirectory(libs/rapidyaml)

add_subdirectory(libs/yaml-cpp)

add_subdirectory(Engine/SEngine)
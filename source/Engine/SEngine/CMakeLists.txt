cmake_minimum_required(VERSION 3.1)

project(SEngine LANGUAGES CXX)
				
add_definitions(-DSE_BUILD)

link_directories("${CMAKE_BINARY_DIR}/lib")
link_directories("${Assimp_BINARY_DIR}/lib")#TODO assimp is a special boy for some reason

file(GLOB SIBERIEN_ENGINE_SRC "*.*")
source_group("SiberienEngine" FILES ${SIBERIEN_ENGINE_SRC})
add_executable(SiberienEngine ${SIBERIEN_ENGINE_SRC})
target_include_directories(SiberienEngine PUBLIC "")


target_link_libraries(SiberienEngine PUBLIC OptickCore)

target_link_libraries(SiberienEngine PUBLIC SDL2)
target_link_libraries(SiberienEngine PUBLIC SDL2main)

target_link_libraries(SiberienEngine PRIVATE assimp)

target_link_libraries(SiberienEngine PUBLIC ryml)
target_link_libraries(SiberienEngine PUBLIC c4core)

#TODO delete lib
target_link_libraries(SiberienEngine PUBLIC yaml-cpp)

target_link_libraries(SiberienEngine PUBLIC zlibstatic)

target_link_libraries(SiberienEngine PUBLIC lz4)
target_include_directories(SiberienEngine PRIVATE "${lz4_SOURCE_DIR}") #TODO why including separately?

target_link_libraries(SiberienEngine PUBLIC Recast)
target_link_libraries(SiberienEngine PUBLIC Detour)
target_link_libraries(SiberienEngine PUBLIC DebugUtils) # TODO bad name
target_link_libraries(SiberienEngine PUBLIC DetourCrowd)
target_link_libraries(SiberienEngine PUBLIC DetourTileCache)

target_link_libraries(SiberienEngine PUBLIC physfs-static)
target_include_directories(SiberienEngine PRIVATE "${PhysicsFS_SOURCE_DIR}/src") #TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC Bullet3Common)
target_link_libraries(SiberienEngine PUBLIC BulletCollision)
target_link_libraries(SiberienEngine PUBLIC Bullet3Dynamics)
target_link_libraries(SiberienEngine PUBLIC BulletDynamics)
target_link_libraries(SiberienEngine PUBLIC LinearMath)
target_link_libraries(SiberienEngine PUBLIC Bullet2FileLoader)
target_include_directories(SiberienEngine PUBLIC "${BULLET_PHYSICS_SOURCE_DIR}/src") #TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC bgfx)
target_include_directories(SiberienEngine PUBLIC "${bgfx_SOURCE_DIR}/../../../../../bgfx/include")#TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC bx)
target_include_directories(SiberienEngine PRIVATE "${bx_SOURCE_DIR}/../../../../../bx/include")#TODO do we need separate include ?
target_include_directories(SiberienEngine PRIVATE "${bx_SOURCE_DIR}/../../../../../bx/include/compat/msvc")#TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC bimg)
target_include_directories(SiberienEngine PRIVATE "${bimg_SOURCE_DIR}/../../../../../bimg/include")#TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC bimg_decode)
target_include_directories(SiberienEngine PRIVATE "${bimg_decode_SOURCE_DIR}/../../../../../bimg_decode/include")#TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC bgfx-example-common)
#TEMP HACKS ?
target_include_directories(SiberienEngine PUBLIC "${SiberienEngine_SOURCE_DIR}/libs/mathfu/include")

target_include_directories(SiberienEngine PRIVATE "${SiberienEngine_SOURCE_DIR}/libs/bgfx/include")
target_include_directories(SiberienEngine PRIVATE "${SiberienEngine_SOURCE_DIR}/libs/bgfx/3rdparty")
target_include_directories(SiberienEngine PRIVATE "${SiberienEngine_SOURCE_DIR}/libs/bgfx/examples/common")

target_link_libraries(SiberienEngine PUBLIC CMP_Compressonator)
target_link_libraries(SiberienEngine PUBLIC CMP_Core)
target_link_libraries(SiberienEngine PUBLIC CMP_Framework)
target_include_directories(SiberienEngine PRIVATE "${Compressonator_SOURCE_DIR}/cmp_compressonatorlib")#TODO do we need separate include ?
target_include_directories(SiberienEngine PRIVATE "${Compressonator_SOURCE_DIR}/cmp_framework/common")#TODO do we need separate include ?

target_link_libraries(SiberienEngine PUBLIC Shlwapi) # for finding tools like blender
target_link_libraries(SiberienEngine PUBLIC winmm) # TODO hack for sdl
target_link_libraries(SiberienEngine PUBLIC imm32) # TODO hack for sdl
target_link_libraries(SiberienEngine PUBLIC setupapi) # TODO hack for sdl
target_link_libraries(SiberienEngine PUBLIC version) # TODO hack for sdl


# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
    # message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
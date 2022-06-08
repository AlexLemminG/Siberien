cmake_minimum_required(VERSION 3.1)

project(SEngine)
# add_executable(SiberienEngine
				# Main.cpp
				# Animation.cpp)
				
add_definitions(-DSE_BUILD)

link_directories("${CMAKE_BINARY_DIR}/lib")
link_directories("${Assimp_BINARY_DIR}/lib")#TODO assimp is a special boy for some reason

file(GLOB SIBERIEN_ENGINE_SRC "*.*")
source_group("SiberienEngine" FILES ${SIBERIEN_ENGINE_SRC})
add_executable(SiberienEngine ${SIBERIEN_ENGINE_SRC})

target_link_libraries(SiberienEngine PRIVATE OptickCored)
target_include_directories(SiberienEngine PUBLIC "${Optick_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PRIVATE debug SDL2d)
target_link_libraries(SiberienEngine PRIVATE debug SDL2maind)
target_include_directories(SiberienEngine PUBLIC "${SDL2_SOURCE_DIR}/include")

#TODO looks strange
target_link_libraries(SiberienEngine PRIVATE assimp-vc142-mtd)
target_include_directories(SiberienEngine PRIVATE "${Assimp_SOURCE_DIR}/include")

target_link_libraries(SiberienEngine PRIVATE rymld)
target_include_directories(SiberienEngine PUBLIC "${ryml_SOURCE_DIR}/src")
target_link_libraries(SiberienEngine PRIVATE c4cored)
target_include_directories(SiberienEngine PUBLIC "${c4core_SOURCE_DIR}/src")

#TODO delete lib
target_link_libraries(SiberienEngine PUBLIC yaml-cppd)
target_include_directories(SiberienEngine PRIVATE "${YAML_CPP_SOURCE_DIR}/include")

target_link_libraries(SiberienEngine PUBLIC zlibstaticd)
target_include_directories(SiberienEngine PRIVATE "${zlib_SOURCE_DIR}")

target_link_libraries(SiberienEngine PUBLIC lz4d)
target_include_directories(SiberienEngine PRIVATE "${lz4_SOURCE_DIR}")

target_link_libraries(SiberienEngine PUBLIC Recast-d)
target_link_libraries(SiberienEngine PUBLIC Detour-d)
target_link_libraries(SiberienEngine PUBLIC DebugUtils-d) # TODO bad name
target_link_libraries(SiberienEngine PUBLIC DetourCrowd-d)
target_link_libraries(SiberienEngine PUBLIC DetourTileCache-d)
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/Detour/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/DetourCrowd/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/Recast/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/DebugUtils/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/DetourTileCache/Include")

target_link_libraries(SiberienEngine PUBLIC physfs-staticd)
target_include_directories(SiberienEngine PRIVATE "${PhysicsFS_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC Bullet3Commond)
target_link_libraries(SiberienEngine PUBLIC BulletCollisiond)
target_link_libraries(SiberienEngine PUBLIC Bullet3Dynamicsd)
target_link_libraries(SiberienEngine PUBLIC BulletDynamicsd)
target_link_libraries(SiberienEngine PUBLIC LinearMathd)
target_link_libraries(SiberienEngine PUBLIC Bullet2FileLoaderd)
target_include_directories(SiberienEngine PUBLIC "${BULLET_PHYSICS_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC bgfxd)
target_include_directories(SiberienEngine PUBLIC "${bgfx_SOURCE_DIR}/../../../../../bgfx/include")

target_link_libraries(SiberienEngine PUBLIC bxd)
target_include_directories(SiberienEngine PRIVATE "${bx_SOURCE_DIR}/../../../../../bx/include")
target_include_directories(SiberienEngine PRIVATE "${bx_SOURCE_DIR}/../../../../../bx/include/compat/msvc")

target_link_libraries(SiberienEngine PUBLIC bimgd)
target_include_directories(SiberienEngine PRIVATE "${bimg_SOURCE_DIR}/../../../../../bimg/include")

target_link_libraries(SiberienEngine PUBLIC bimg_decoded)
target_include_directories(SiberienEngine PRIVATE "${bimg_decode_SOURCE_DIR}/../../../../../bimg_decode/include")

target_link_libraries(SiberienEngine PUBLIC bgfx-example-commond)
#TEMP HACKS ?
target_include_directories(SiberienEngine PUBLIC "${SiberienEngine_SOURCE_DIR}/libs/mathfu/include")

target_include_directories(SiberienEngine PRIVATE "${SiberienEngine_SOURCE_DIR}/libs/bgfx/include")
target_include_directories(SiberienEngine PRIVATE "${SiberienEngine_SOURCE_DIR}/libs/bgfx/3rdparty")
target_include_directories(SiberienEngine PRIVATE "${SiberienEngine_SOURCE_DIR}/libs/bgfx/examples/common")

target_link_libraries(SiberienEngine PUBLIC CMP_Compressonatord)
target_link_libraries(SiberienEngine PUBLIC CMP_Cored)
target_link_libraries(SiberienEngine PUBLIC CMP_Frameworkd)
target_include_directories(SiberienEngine PRIVATE "${Compressonator_SOURCE_DIR}/cmp_compressonatorlib")
target_include_directories(SiberienEngine PRIVATE "${Compressonator_SOURCE_DIR}/cmp_framework/common")

target_link_libraries(SiberienEngine PUBLIC Shlwapi) # for finding tools like blender
target_link_libraries(SiberienEngine PUBLIC winmm) # TODO hack for sdl
target_link_libraries(SiberienEngine PUBLIC imm32) # TODO hack for sdl
target_link_libraries(SiberienEngine PUBLIC setupapi) # TODO hack for sdl
target_link_libraries(SiberienEngine PUBLIC version) # TODO hack for sdl

# include_external_msproject(bgfx ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/projects/vs2019/bgfx.vcxproj)
# include_external_msproject(bx ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/projects/vs2019/bx.vcxproj)
# include_external_msproject(bimg ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/projects/vs2019/bimg.vcxproj)
# target_link_libraries(SiberienEngine PUBLIC ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/win64_vs2019/bin/bx$<CONFIG>.lib)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
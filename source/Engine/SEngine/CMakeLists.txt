# add_executable(SiberienEngine
				# Main.cpp
				# Animation.cpp)
				
add_definitions(-DSE_BUILD)

link_directories("${CMAKE_BINARY_DIR}/lib")

file(GLOB SIBERIEN_ENGINE_SRC "*.*")
source_group("SiberienEngine" FILES ${SIBERIEN_ENGINE_SRC})
add_executable(SiberienEngine ${SIBERIEN_ENGINE_SRC})

target_link_libraries(SiberienEngine PUBLIC OptickCored)
target_include_directories(SiberienEngine PRIVATE "${Optick_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC SDL2)
target_include_directories(SiberienEngine PRIVATE "${SDL2_SOURCE_DIR}/include")

#TODO looks strange
# link_directories("${Assimp_BINARY_DIR}/lib")
# target_link_libraries(SiberienEngine PUBLIC assimp-vc142-mtd)
target_include_directories(SiberienEngine PRIVATE "${Assimp_SOURCE_DIR}/include")

target_link_libraries(SiberienEngine PUBLIC ryml)
target_include_directories(SiberienEngine PRIVATE "${ryml_SOURCE_DIR}/src")
target_link_libraries(SiberienEngine PUBLIC c4core)
target_include_directories(SiberienEngine PRIVATE "${c4core_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC yaml-cppd)
target_include_directories(SiberienEngine PRIVATE "${YAML_CPP_SOURCE_DIR}/include")

target_link_libraries(SiberienEngine PUBLIC zlibstaticd)
target_include_directories(SiberienEngine PRIVATE "${zlib_SOURCE_DIR}")

target_link_libraries(SiberienEngine PUBLIC lz4)
target_include_directories(SiberienEngine PRIVATE "${lz4_SOURCE_DIR}")

target_link_libraries(SiberienEngine PUBLIC Recast-d)
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/Detour/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/DetourCrowd/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/Recast/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/DebugUtils/include")
target_include_directories(SiberienEngine PRIVATE "${RecastNavigation_SOURCE_DIR}/DetourTileCache/Include")

target_link_libraries(SiberienEngine PUBLIC physfsd)
target_include_directories(SiberienEngine PRIVATE "${PhysicsFS_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC Bullet3Commond)
target_link_libraries(SiberienEngine PUBLIC BulletCollisiond)
target_link_libraries(SiberienEngine PUBLIC Bullet3Dynamicsd)
target_link_libraries(SiberienEngine PUBLIC LinearMathd)
target_link_libraries(SiberienEngine PUBLIC Bullet2FileLoaderd)
target_include_directories(SiberienEngine PRIVATE "${BULLET_PHYSICS_SOURCE_DIR}/src")

# target_link_libraries(SiberienEngine PUBLIC bgfx)
# target_include_directories(SiberienEngine PRIVATE "${bgfx_SOURCE_DIR}/../../../../../bgfx/include")

# target_link_libraries(SiberienEngine PUBLIC bx)
target_include_directories(SiberienEngine PRIVATE "${bx_SOURCE_DIR}/../../../../../bx/include")
target_include_directories(SiberienEngine PRIVATE "${bx_SOURCE_DIR}/../../../../../bx/include/compat/msvc")

# target_link_libraries(SiberienEngine PUBLIC bimg)
target_include_directories(SiberienEngine PRIVATE "${bimg_SOURCE_DIR}/../../../../../bimg/include")

#TEMP HACKS ?
target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/mathfu/include")

target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/bgfx/include")
target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/bgfx/3rdparty")
target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/bgfx/examples/common")

target_link_libraries(SiberienEngine PUBLIC CMP_Compressonatord)
target_include_directories(SiberienEngine PRIVATE "${Compressonator_SOURCE_DIR}/cmp_compressonatorlib")
target_include_directories(SiberienEngine PRIVATE "${Compressonator_SOURCE_DIR}/cmp_framework/common")

include_external_msproject(bgfx ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/projects/vs2019/bgfx.vcxproj)
include_external_msproject(bx ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/projects/vs2019/bx.vcxproj)
include_external_msproject(bimg ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/projects/vs2019/bimg.vcxproj)
# target_link_libraries(SiberienEngine PUBLIC ${PROJECT_SOURCE_DIR}/libs/bgfx/.build/win64_vs2019/bin/bx$<CONFIG>.lib)

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
    # message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
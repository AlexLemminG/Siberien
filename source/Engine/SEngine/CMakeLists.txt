# add_executable(SiberienEngine
				# Main.cpp
				# Animation.cpp)
				

file(GLOB SIBERIEN_ENGINE_SRC "*.*")
source_group("SiberienEngine" FILES ${SIBERIEN_ENGINE_SRC})
add_executable(SiberienEngine ${SIBERIEN_ENGINE_SRC})

target_link_libraries(SiberienEngine PUBLIC optick)
target_include_directories(SiberienEngine PRIVATE "${Optick_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC SDL2)
target_include_directories(SiberienEngine PRIVATE "${SDL2_SOURCE_DIR}/include")

target_link_libraries(SiberienEngine PUBLIC Assimp)
target_include_directories(SiberienEngine PRIVATE "${Assimp_SOURCE_DIR}/include")

target_link_libraries(SiberienEngine PUBLIC ryml)
target_include_directories(SiberienEngine PRIVATE "${ryml_SOURCE_DIR}/src")

target_link_libraries(SiberienEngine PUBLIC yamlcpp)
target_include_directories(SiberienEngine PRIVATE "${YAML_CPP_SOURCE_DIR}/include")

#TEMP HACKS ?
target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/mathfu/include")

target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/bgfx/include")
target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/bgfx/3rdparty")
target_include_directories(SiberienEngine PRIVATE "${PROJECT_SOURCE_DIR}/libs/bgfx/examples/common")

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
    # message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
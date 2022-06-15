cmake_minimum_required(VERSION 3.10)
project(lz4 VERSION 1.0.1 DESCRIPTION "mylib description")
# include(GNUInstallDirs)


file(GLOB LZ4_SRC "*.*")
source_group("LZ4Source" FILES ${LZ4_SRC})

file(GLOB LZ4_SRC "*.*")
source_group("LZ4Headers" FILES ${LZ4_HDR})

add_library(lz4 SHARED ${LZ4_SRC})
set_target_properties(lz4 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ${LZ4_HDR})

# configure_file(liblz4.pc.cmakein liblz4.pc @ONLY)

target_include_directories(lz4 PUBLIC .)
install(TARGETS lz4
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
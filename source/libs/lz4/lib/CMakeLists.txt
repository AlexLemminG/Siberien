cmake_minimum_required(VERSION 3.10)
project(lz4 VERSION 1.0.1 DESCRIPTION "mylib description")
# include(GNUInstallDirs)

file(GLOB LZ4_SRC "*.c")
source_group("LZ4Source" FILES ${LZ4_SRC})

file(GLOB LZ4_HDR "*.h")
source_group("LZ4Headers" FILES ${LZ4_HDR})

add_library(lz4 STATIC ${LZ4_SRC} ${LZ4_HDR})

set_target_properties(lz4 PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(lz4 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1)
configure_file(liblz4.pc.in liblz4.pc @ONLY)
target_include_directories(lz4 PRIVATE .)
install(TARGETS lz4
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/lz4.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
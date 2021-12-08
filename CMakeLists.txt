cmake_minimum_required (VERSION 3.8)

project ("cpymo")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

add_definitions (-D_CRT_SECURE_NO_WARNINGS)

include_directories ("./stb/")
include_directories ("./endianness.h/")
include_directories ("./utf8.h/")

add_subdirectory ("cpymo")
add_subdirectory ("cpymo-tool")
add_subdirectory ("cpymo-backends/sdl2")

cmake_minimum_required (VERSION 3.8)

project ("cpymo")

add_definitions (-D_CRT_SECURE_NO_WARNINGS)

include_directories ("./endianness.h/")

add_subdirectory ("cpymo")
add_subdirectory ("cpymo-tool")


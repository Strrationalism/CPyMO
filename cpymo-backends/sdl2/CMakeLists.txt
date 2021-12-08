cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")

link_libraries (cpymolib)
add_executable(cpymo "main.c")
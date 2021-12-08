cmake_minimum_required (VERSION 3.8)

add_library (cpymolib STATIC
	"cpymo_error.h" "cpymo_error.c"
	"cpymo_package.h" "cpymo_package.c")

link_libraries (cpymolib)

add_executable (cpymo "main.c")
cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

add_library (libcpymotool STATIC
	"cpymo_tool_package.h" "cpymo_tool_package.c")

link_libraries (libcpymo libcpymotool)

add_executable (cpymo-tool 
	"main.c" )
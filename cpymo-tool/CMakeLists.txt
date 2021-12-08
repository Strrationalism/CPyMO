cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

add_library (libcpymotool STATIC
	"cpymo_tool_package.h" "cpymo_tool_package.c")

add_executable (cpymo-tool 
	"main.c" )

target_link_libraries (cpymo-tool libcpymo)
target_link_libraries (cpymo-tool libcpymotool)
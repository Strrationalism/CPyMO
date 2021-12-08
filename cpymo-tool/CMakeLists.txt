cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

add_library (cpymotool STATIC
	"cpymo_tool_package.h" "cpymo_tool_package.c")

include_directories ("../cpymo")

add_executable (cpymo-tool
	"main.c" )

add_subdirectory ("../cpymo")
target_link_libraries (cpymo-tool cpymotool)

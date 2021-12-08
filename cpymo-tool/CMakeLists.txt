cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")
link_libraries (libcpymo)

add_executable (cpymo-tool 
	"package.h" "package.c"
	"main.c" )




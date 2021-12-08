cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

aux_source_directory("." SRC)

set (CPYMO_COMPONENTS
	"../cpymo/cpymo_error.h" "../cpymo/cpymo_error.c"
	"../cpymo/cpymo_package.h" "../cpymo/cpymo_package.c")

add_executable (cpymo-tool
	${SRC})

target_link_libraries (cpymo-tool cpymolib)

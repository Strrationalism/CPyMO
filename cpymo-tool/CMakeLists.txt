cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

aux_source_directory("." SRC)

set (CPYMO_COMPONENTS
	"../cpymo/cpymo_error.h" "../cpymo/cpymo_error.c"
	"../cpymo/cpymo_package.h" "../cpymo/cpymo_package.c"
	"../cpymo/cpymo_parser.h" "../cpymo/cpymo_parser.c"
	"../cpymo/cpymo_utils.h" "../cpymo/cpymo_utils.c"
	"../cpymo/cpymo_color.h" "../cpymo/cpymo_color.c")

add_executable (cpymo-tool
	${SRC} ${CPYMO_COMPONENTS})



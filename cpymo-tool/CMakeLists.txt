cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

file (GLOB CPYMO_COMPONENTS "*.h" "*.c")


add_executable (cpymo-tool
	"../cpymo/cpymo_error.c"
	"../cpymo/cpymo_package.c"
	"../cpymo/cpymo_parser.c"
	"../cpymo/cpymo_utils.c"
	"../cpymo/cpymo_color.c"
	${CPYMO_COMPONENTS})


if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_link_libraries(cpymo-tool m)
endif ()
cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

file (GLOB CPYMO_COMPONENTS "*.h" "*.c")


if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_link_libraries(cpymo-tool m)
endif ()
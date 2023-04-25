cmake_minimum_required (VERSION 3.8)

include_directories ("../cpymo")

file (GLOB CPYMO_TOOL_SRC "*.h" "*.c")
add_executable (cpymo-tool ${CPYMO_TOOL_SRC})

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_link_libraries(cpymo-tool m)
endif ()

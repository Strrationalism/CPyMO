cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")

find_package (SDL2 CONFIG REQUIRED)
find_package (FFMPEG REQUIRED)

file (GLOB CPYMO_BACKEND_SRC "*.h" "*.c")

add_executable (cpymo ${CPYMO_BACKEND_SRC})

target_link_libraries(cpymo PRIVATE 
	SDL2::SDL2 
	SDL2::SDL2main 
	cpymolib
	${FFMPEG_LIBRARIES})


if (VCPKG_LIBRARY_LINKAGE EQUAL "static")
	target_link_libraries(cpymo PRIVATE SDL2::SDL2-static)
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_sources (cpymo PRIVATE "pymo-icon-windows.rc")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_C_FLAGS "-D__LINUX__")
    target_link_libraries(cpymo PRIVATE X11)
endif ()


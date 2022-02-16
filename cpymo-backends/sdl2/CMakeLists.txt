cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")


find_package (SDL2 CONFIG REQUIRED)
find_package (FFMPEG REQUIRED)

add_executable (cpymo 
	"main.c" 
	"cpymo_backend_image.c" 
	"cpymo_backend_input.c" 
	"cpymo_backend_font.c" 
	"cpymo_backend_masktrans.c" 
	"cpymo_backend_text.c" 
	"posix_win32.h" 
	"cpymo_backend_save.c" 
	"cpymo_backend_audio.c")

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_sources (cpymo PRIVATE "pymo-icon-windows.rc")
endif (CMAKE_SYSTEM_NAME MATCHES "Windows")

target_link_libraries(cpymo PRIVATE 
	SDL2::SDL2 
	SDL2::SDL2main 
	SDL2::SDL2-static 
	cpymolib
	${FFMPEG_LIBRARIES})
	
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(cpymo PRIVATE X11)
endif (CMAKE_SYSTEM_NAME MATCHES "Linux")


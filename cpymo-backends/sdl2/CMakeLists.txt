cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")


add_subdirectory (SDL)
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
	"cpymo_backend_audio.c" 
	"cpymo_backend_movie.c" 
	"cpymo_backend_audio_sdl2_mixer.c")

target_link_libraries(cpymo PRIVATE 
	SDL2::SDL2-static
	SDL2::SDL2main 
	cpymolib
	${FFMPEG_LIBRARIES})


if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_sources (cpymo PRIVATE "pymo-icon-windows.rc")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_C_FLAGS "-D__LINUX__")
    target_link_libraries(cpymo PRIVATE X11)
endif ()


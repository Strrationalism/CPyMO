cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")


find_package(SDL2 CONFIG REQUIRED)

add_executable(cpymo 
	"main.c" 
	"cpymo_backend_image.c" 
	"cpymo_backend_input.c" 
	"cpymo_backend_font.c" 
	"cpymo_backend_masktrans.c" 
	"cpymo_backend_text.c" 
	"posix_win32.h" 
	"cpymo_backend_save.c" "cpymo_backend_audio.c")

target_link_libraries(cpymo PRIVATE 
	SDL2::SDL2 
	SDL2::SDL2main 
	SDL2::SDL2-static 
	cpymolib)


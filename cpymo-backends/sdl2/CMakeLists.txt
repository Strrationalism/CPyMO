cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")

add_subdirectory ("SDL")

include_directories ("SDL2/include")
link_libraries (cpymolib SDL2::SDL2main SDL2::SDL2-static)
add_executable(cpymo "main.c" "cpymo_backend_image.c" "cpymo_backend_input.c" "cpymo_backend_font.c" "cpymo_backend_masktrans.c")
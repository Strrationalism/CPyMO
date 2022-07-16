cmake_minimum_required (VERSION 3.8)

include_directories (${FFMPEG_INCLUDE_DIRS})

file (GLOB CPYMO_SRC "*.h" "*.c")

add_library (cpymolib STATIC ${CPYMO_SRC})


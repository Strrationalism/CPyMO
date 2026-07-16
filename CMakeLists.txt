cmake_minimum_required (VERSION 3.8)

project ("cpymo")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

add_definitions (-D_CRT_SECURE_NO_WARNINGS)
add_definitions (-DENABLE_TEXT_EXTRACT)
add_definitions (-DENABLE_TEXT_EXTRACT_COPY_TO_CLIPBOARD)
add_definitions (-DENABLE_EXIT_CONFIRM)
add_definitions (-DLEAKCHECK)
add_definitions (-DLIMIT_WINDOW_SIZE_TO_SCREEN)
if(MSVC OR WIN32)
    find_package(FFMPEG REQUIRED)
else()
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libswresample)
endif()

if (FFMPEG_FOUND)
else ()
	MESSAGE ("Can not find FFMPEG!")
endif (FFMPEG_FOUND)

include_directories (${FFMPEG_INCLUDE_DIRS})

add_subdirectory ("cpymo")
add_subdirectory ("cpymo-tool")
add_subdirectory ("cpymo-backends/sdl2")

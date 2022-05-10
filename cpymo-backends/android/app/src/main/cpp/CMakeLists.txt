# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

set(CMAKE_C_FLAGS "-O3 -DNDEBUG ${CMAKE_C_FLAGS}")

add_subdirectory(../../../../../sdl2/SDL SDL)

project("cpymo")

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


add_definitions(
        -DUSE_GAME_SELECTOR
        -DADAPT_SCREEN_SIZE
        -DSCREEN_WIDTH=800
        -DSCREEN_HEIGHT=600
        -D__ANDROID__
        -DGAME_SELECTOR_FONTSIZE=46
        -DNON_VISUALLY_IMPAIRED_HELP
        "-DGAME_SELECTOR_EMPTY_MSG_FONTSIZE=56"
        -DGAME_SELECTOR_COUNT_PER_SCREEN=4
        -DGAME_SELECTOR_DIR="/sdcard/pymogames"
)

include_directories(
        "../../../../../include"
        "../../../../../../cpymo"
        "../../../../../../stb"
        "../../../../../../endianness.h"
        "../../../../ffmpeg-android-maker/output/include/${ANDROID_ABI}/")

add_subdirectory("../../../../../../cpymo" cpymolib)

add_library( # Sets the name of the library.
        cpymo

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).

        "../../../../../sdl2/main.c"
        "../../../../../sdl2/cpymo_backend_image.c"
        "../../../../../sdl2/cpymo_backend_input.c"
        "../../../../../sdl2/cpymo_backend_font.c"
        "../../../../../sdl2/cpymo_backend_masktrans.c"
        "../../../../../sdl2/cpymo_backend_text.c"
        "../../../../../sdl2/posix_win32.h"
        "../../../../../sdl2/cpymo_backend_save.c"
        "../../../../../sdl2/cpymo_backend_audio.c"
        "../../../../../sdl2/cpymo_backend_movie.c"
        "../../../../../sdl2/cpymo_import_sdl2.h")

link_directories("../../../../ffmpeg-android-maker/output/lib/${ANDROID_ABI}/")

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/../../../../ffmpeg-android-maker/output/lib/${ANDROID_ABI}/libavformat.so")

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/../../../../ffmpeg-android-maker/output/lib/${ANDROID_ABI}/libavcodec.so")

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/../../../../ffmpeg-android-maker/output/lib/${ANDROID_ABI}/libavutil.so")

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/../../../../ffmpeg-android-maker/output/lib/${ANDROID_ABI}/libswresample.so")

target_link_libraries(cpymo PUBLIC
        SDL2main SDL2 cpymolib
        avformat avcodec avutil swresample
        ${log-lib} c++_static)

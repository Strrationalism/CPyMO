# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.


set(WITH_OGG OFF CACHE STRING "FLAC WITH OGG")   # WTF in FLAC???
set(SUPPORT_WAV ON CACHE STRING "SUPPORT_WAV")
set(SUPPORT_FLAC ON CACHE STRING "SUPPORT_FLAC" FORCE)
set(SUPPORT_OGG ON CACHE STRING "SUPPORT_OGG")
set(SUPPORT_MP3_MPG123 ON CACHE STRING "SUPPORT_MP3_MPG123")
set(SUPPORT_MOD_MODPLUG ON CACHE STRING "SUPPORT_MOD_MODPLUG")
#set(SUPPORT_MID_TIMIDITY ON CACHE STRING "SUPPORT_MID_TIMIDITY")

add_subdirectory(../../../../../sdl2/SDL SDL)
add_subdirectory(../../../../../sdl2/SDL_mixer SDL_mixer)

project("cpymo")

#find_package(SDL2 CONFIG REQUIRED)
#find_package(sdl2-mixer CONFIG REQUIRED)

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
        -DENABLE_SDL2_MIXER_AUDIO_BACKEND
        -DDISABLE_FFMPEG_AUDIO
        #-DDISABLE_AUDIO
        -DDISABLE_MOVIE
        -DUSE_GAME_SELECTOR
        -DADAPT_SCREEN_SIZE
        -DSCREEN_WIDTH=800
        -DSCREEN_HEIGHT=600
        -DNDEBUG
        -D__ANDROID__
        -DGAME_SELECTOR_FONTSIZE=46
        "-DGAME_SELECTOR_EMPTY_MSG_FONTSIZE=56"
        -DGAME_SELECTOR_COUNT_PER_SCREEN=4
        -DGAME_SELECTOR_DIR="/sdcard/pymogames"
)

include_directories(
        "../../../../../include"
        "../../../../../../cpymo"
        "../../../../../../stb"
        "../../../../../../endianness.h")

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
        "../../../../../sdl2/cpymo_backend_audio_sdl2_mixer.c")

target_link_libraries(cpymo PUBLIC SDL2main SDL2 SDL2_mixer cpymolib ${log-lib})

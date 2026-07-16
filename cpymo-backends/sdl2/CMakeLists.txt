cmake_minimum_required (VERSION 3.8)

include_directories("../../cpymo")

find_package (SDL2 CONFIG REQUIRED)

if (MSVC OR WIN32)
    find_package (FFMPEG REQUIRED)
endif()

include_directories(${FFMPEG_INCLUDE_DIRS})

file (GLOB CPYMO_BACKEND_SRC "*.h" "*.c")

add_executable (cpymo ${CPYMO_BACKEND_SRC})

target_link_libraries(cpymo PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    cpymolib
    ${FFMPEG_LIBRARIES}
)

if (VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    target_link_libraries(cpymo PRIVATE SDL2::SDL2-static)
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_sources (cpymo PRIVATE "pymo-icon-windows.rc")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(cpymo PRIVATE __LINUX__)

    execute_process(
        COMMAND pkg-config --libs --static libavformat libavcodec libavutil libswscale libswresample
        OUTPUT_VARIABLE FFMPEG_STATIC_LIBS_STR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    separate_arguments(FFMPEG_FULL_LIBS_LIST UNIX_COMMAND "${FFMPEG_STATIC_LIBS_STR}")

    target_link_libraries(cpymo PRIVATE X11 ${FFMPEG_FULL_LIBS_LIST})
endif ()

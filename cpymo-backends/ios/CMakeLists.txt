cmake_minimum_required(VERSION 3.18.1)
project(cpymo
        VERSION 1.1.4
        LANGUAGES C OBJC
        )

# fix compile error for `SDL_systimer.c`
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wno-error=declaration-after-statement")

find_library(LIBZ z)
find_library(LIBBZ2 bz2)
find_library(LIBICONV iconv)
find_library(SECURITY_LIBRARY Security)
find_library(COREMEDIA_LIBRARY CoreMedia)
find_library(COREVIDEO_LIBRARY CoreVideo)
find_library(FOUNDATION_LIBRARY Foundation)
find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
find_library(VIDEOTOOLBOX_LIBRARY VideoToolbox)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)

set(BUILD_SHARED_LIBS OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)

add_subdirectory(../sdl2/SDL SDL)

add_definitions(
        -D__IOS__
        -DUSE_GAME_SELECTOR
        -DADAPT_SCREEN_SIZE
        -DSCREEN_WIDTH=800
        -DSCREEN_HEIGHT=600
        -DGAME_SELECTOR_FONTSIZE=46
        -DGAME_SELECTOR_EMPTY_MSG_FONTSIZE=56
        -DGAME_SELECTOR_COUNT_PER_SCREEN=4
        -DEXIT_TO_GAME_SELECTOR
)

include_directories(
        "../include"
        "../../cpymo"
        "../../stb"
        "../../endianness.h"
        "./FFmpeg-iOS-build-script/thin/arm64/include"
)

add_subdirectory("../../cpymo" cpymolib)

file(GLOB CPYMO_BACKEND_SRC "../sdl2/*.c")

add_executable(
        cpymo
        cpymo_ios.m
        ${CPYMO_SRC}
        ${CPYMO_BACKEND_SRC}
        Packaging/AppIcon.icns
        Packaging/LaunchScreen.storyboard
#        ../sdl2/SDL/src/main/uikit/SDL_uikit_main.c
)

set_source_files_properties("./Packaging/AppIcon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set(MACOSX_BUNDLE_GUI_IDENTIFIER xyz.xydm.cpymo)
set(MACOSX_BUNDLE_COPYRIGHT Unlicense)
set(MACOSX_BUNDLE_BUNDLE_NAME cpymo)
set(MACOSX_BUNDLE_DISPLAY_NAME CPyMO)
set(MACOSX_BUNDLE_INFO_STRING ${PROJECT_VERSION})
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
set(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${PROJECT_VERSION}")
set(MACOSX_BUNDLE_REQUIRED_PLATFORM IPhoneOS)
set_target_properties(cpymo PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
set(CMAKE_OSX_DEPLOYMENT_TARGET "9.0")
set_target_properties(cpymo PROPERTIES MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
set_target_properties(cpymo PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Packaging/Info.plist")

install(TARGETS cpymo DESTINATION ./)

install(CODE "
        include(BundleUtilities)
        fixup_bundle(${CMAKE_BINARY_DIR}/${MACOSX_BUNDLE_BUNDLE_NAME}.app \"\" \"\")
        "
        COMPONENT Runtime)

set(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${PROJECT_VERSION}")
set(CPACK On)

link_directories("./FFmpeg-iOS-build-script/thin/arm64/lib")

add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/FFmpeg-iOS-build-script/thin/arm64/lib/libavformat.a")

add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/FFmpeg-iOS-build-script/thin/arm64/lib/libavcodec.a")

add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/FFmpeg-iOS-build-script/thin/arm64/lib/libavutil.a")

add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/FFmpeg-iOS-build-script/thin/arm64/lib/libswresample.a")

target_link_libraries(cpymo PRIVATE
        SDL2main SDL2-static cpymolib
        avformat avcodec avutil swresample
        )

target_link_libraries(cpymo PUBLIC
        ${LIBZ}
        ${LIBBZ2}
        ${LIBICONV}
        ${SECURITY_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${FOUNDATION_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${VIDEOTOOLBOX_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        )

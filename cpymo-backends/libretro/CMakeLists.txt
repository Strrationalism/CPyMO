cmake_minimum_required(VERSION 3.9)
project(cpymo-libretro C)

option(LIBRETRO_STATIC "Statically link the libretro core" OFF)

set(SOURCES
  libretro.c
)
aux_source_directory(../../cpymo SOURCES)
aux_source_directory(../software SOURCES)

if (LIBRETRO_STATIC)
  add_library(cpymo_libretro STATIC ${SOURCES})
else ()
  add_library(cpymo_libretro SHARED ${SOURCES})
endif ()

target_compile_definitions(cpymo_libretro PRIVATE
  -DTEXT_CHARACTER_W_SCALE=1
)
target_link_options(cpymo_libretro PRIVATE
  -Wl,--version-script=${CMAKE_SOURCE_DIR}/link.T
  -Wl,--no-undefined -lm
  # TODO: link ffmpeg statically
  -lavcodec -lavformat -lavutil -lswresample -lswscale
)

# Follow naming conventions for libretro cores
set_target_properties(cpymo_libretro PROPERTIES PREFIX "")

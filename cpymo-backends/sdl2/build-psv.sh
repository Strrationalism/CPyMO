    #!/bin/sh

BUILD_DIR="build.PSV"

CC=arm-vita-eabi-gcc

if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

CompileC()
{
    SRC_PATH=$1
    SRC_NAME=${SRC_PATH##*/}
    OBJ_PATH=$BUILD_DIR/$SRC_NAME.o
    echo "$SRC_NAME"
    $CC -c -Wl,-q $SRC_PATH -o $OBJ_PATH \
        -I../include \
        -I../../cpymo \
        -I../../stb \
        -I../../endianness.h \
        -I$VITASDK/arm-vita-eabi/include \
        -I$VITASDK/arm-vita-eabi/include/SDL2 \
        -D__PSV__ \
        -DDISABLE_MOVIE \
		-DDISABLE_FFMPEG_AUDIO \
		-DENABLE_SDL2_MIXER_AUDIO_BACKEND \
		-DUSE_GAME_SELECTOR \
        -DGAME_SELECTOR_DIR="\"ux0:/pymogames/\"" \
        -DGAME_SELECTOR_FONTSIZE=24\
        -DGAME_SELECTOR_EMPTY_MSG_FONTSIZE=28 \
        -DGAME_SELECTOR_COUNT_PER_SCREEN=3 \
        -DSCREEN_WIDTH=960 \
        -DSCREEN_HEIGHT=544 \
        -O3 \
        -Wall
}

CompileCPyMO()
{
    for SRC in `ls *.c` 
    do
        CompileC $SRC
    done

    for SRC in `ls ../../cpymo/*.c`
    do
        CompileC $SRC
    done
}

LinkCPyMO()
{
    OBJS=`ls $BUILD_DIR/*.o`
    $CC -Wl,-q $OBJS -o $BUILD_DIR/CPyMO.elf \
        -Xlinker "-(" \
        -lswscale \
        -lavformat \
        -lavcodec \
        -lavutil \
        -lswresample \
        -lSDL2 \
        -lSDL2main \
        -lSceDisplay_stub \
        -lSceCtrl_stub \
        -lSceAudio_stub \
        -lSceAudioIn_stub \
        -lSceSysmodule_stub \
        -lSceGxm_stub \
        -lSceCommonDialog_stub \
        -lSceAppMgr_stub \
        -lSceTouch_stub \
        -lSceHid_stub \
        -lSceMotion_stub \
        -lpthread \
        -lSDL2_mixer \
        -lmikmod \
		-lFLAC \
		-logg \
		-lmpg123 \
        -lvorbis \
        -lvorbisfile \
		-lz \
        -lm \
        -Xlinker "-)" \
        -L$VITASDK/arm-vita-eabi/lib \
        -Wl,-z,nocopyreloc
}

CompileCPyMO
LinkCPyMO
arm-vita-eabi-strip -g $BUILD_DIR/CPyMO.elf
vita-elf-create $BUILD_DIR/CPyMO.elf $BUILD_DIR/CPyMO.velf
vita-make-fself -s -ss $BUILD_DIR/CPyMO.velf eboot.bin
#vita-mksfoex -s TITLE_ID="VSDK03098" "CPyMO" $BUILD_DIR/param.sfo

#vita-pack-vpk -s $BUILD_DIR/param.sfo -b $BUILD_DIR/eboot.bin \
#    --add sce_sys/icon0.png=sce_sys/icon0.png \
#    --add sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
#    --add sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
#    --add sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
#    cpymo.vpk
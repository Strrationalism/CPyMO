#!/bin/sh

curl https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n5.0.tar.gz --output ffmpeg-5.0.tar.gz
tar -xf ffmpeg-5.0.tar.gz

cd ffmpeg-5.0

export PATH=$DEVKITARM/bin:$PATH
export ARCH="-march=armv6k -mtune=mpcore -mfloat-abi=hard"

chmod +x ./configure

./configure --prefix=$(pwd)/../build.FFmpeg.3DS \
--enable-cross-compile \
--cross-prefix=$DEVKITARM/bin/arm-none-eabi- \
--disable-shared \
--disable-runtime-cpudetect \
--disable-armv5te \
--disable-programs \
--disable-network \
--disable-doc \
--disable-encoders \
--disable-filters \
--disable-indevs \
--disable-protocols \
--enable-bsfs \
--disable-muxers \
--enable-protocol=file \
--enable-static \
--disable-decoder=gif,webp,bmp,dds,mjpeg,jpeg1s,jpeg2000,photocd,png,ppm,psd,qpeg,sga,sgi,smvjpeg,text \
--disable-parser=bmp,gif,mjpeg,webp,png,ipu,jpeg2000,pnm \
--arch=armv6k \
--cpu=mpcore \
--disable-armv6t2 \
--disable-neon \
--target-os=none \
--extra-cflags=" -O3 -DARM11 -D_3DS -mword-relocations -fomit-frame-pointer -ffast-math $ARCH" \
--extra-cxxflags=" -O3 -DARM11 -D_3DS -mword-relocations -fomit-frame-pointer -ffast-math -fno-rtti -fno-exceptions -std=gnu++11 $ARCH" \
--extra-ldflags=" -specs=3dsx.specs $ARCH -L$DEVKITARM/lib  -L$DEVKITPRO/libctru/lib  -L$DEVKITPRO/portlibs/3ds/lib -lctru " \
--disable-bzlib \
--disable-iconv \
--disable-lzma \
--disable-securetransport \
--disable-xlib \
--disable-zlib \
--disable-debug \
--enable-gpl \
--enable-version3 

make install -j

cd ..
rm ffmpeg-5.0.tar.gz -f
rm ffmpeg-5.0 -rf

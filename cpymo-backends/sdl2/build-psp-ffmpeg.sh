#!/bin/sh

curl https://codeload.github.com/FFmpeg/FFmpeg/tar.gz/refs/tags/n5.0.1 --output ffmpeg.tar.gz
tar -xf ffmpeg.tar.gz


cd FFmpeg-n5.0.1

chmod +x ./configure
chmod +x ../../../ffmpeg-config.sh

./configure --prefix=$(pwd)/../build.FFmpeg.PSP \
--enable-cross-compile \
--cross-prefix=psp- \
--disable-shared \
--disable-runtime-cpudetect \
--disable-programs \
--disable-network \
--disable-doc \
--disable-encoders \
--disable-filters \
--disable-indevs \
--disable-protocols \
--enable-bsfs \
--enable-hardcoded-tables \
--disable-muxers \
--enable-protocol=file \
--enable-static \
--disable-parsers \
--disable-decoders \
--disable-demuxers \
--disable-avfilter \
--disable-swscale \
--disable-postproc \
--disable-avdevice \
--disable-bsfs \
`../../../ffmpeg-config.sh --audio` \
--arch=mips \
--cpu=generic \
--disable-asm \
--target-os=none \
--extra-cflags=" -O3 -D__PSP__" \
--extra-cxxflags=" -O3 -D__PSP__" \
--disable-bzlib \
--disable-iconv \
--disable-lzma \
--disable-securetransport \
--disable-xlib \
--disable-zlib \
--disable-debug \
--enable-gpl \
--enable-version3 \
--enable-small

make install -j

cd ..
rm -rf FFmpeg-n5.0.1 ffmpeg.tar.gz

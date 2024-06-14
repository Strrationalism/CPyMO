#!/bin/sh

curl https://codeload.github.com/FFmpeg/FFmpeg/tar.gz/refs/tags/n5.0.1 --output ffmpeg.tar.gz
tar -xf ffmpeg.tar.gz


cd FFmpeg-n5.0.1

chmod +x ./configure

PCM_US=pcm_u8,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le
PCM_SS=pcm_s8,pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le
PCM_FS=pcm_f32be,pcm_f32le,pcm_f64le,pcm_f64be
AUDIO_SUPPORT=vorbis,mp3,pcm,ogg,wav,$PCM_US,$PCM_SS,$PCM_FS

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
--enable-decoder=$AUDIO_SUPPORT \
--enable-parser=$AUDIO_SUPPORT \
--enable-demuxer=$AUDIO_SUPPORT \
--arch=mips \
--cpu=generic \
--disable-asm \
--target-os=none \
--extra-cflags=" -O3 -D__PSP__ -Wno-error=incompatible-pointer-types" \
--extra-cxxflags=" -O3 -D__PSP__ -Wno-error=incompatible-pointer-types" \
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

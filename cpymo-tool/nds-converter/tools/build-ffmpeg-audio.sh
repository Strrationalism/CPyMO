#!/usr/bin/env bash
# MIT. Build unmodified official FFmpeg 8.0 as a standalone audio-only program.
set -eu
: "${FFMPEG_SOURCE:?Extract official ffmpeg-8.0.tar.xz and set FFMPEG_SOURCE}"
: "${FFMPEG_BUILD:?Set a separate, new build directory}"
: "${LLVM_MINGW:?Extract llvm-mingw 20261006 ucrt-x86_64 and set LLVM_MINGW}"
export PATH="$LLVM_MINGW/bin:/usr/bin:$PATH"
mkdir -p "$FFMPEG_BUILD"
cd "$FFMPEG_BUILD"
"$FFMPEG_SOURCE/configure" \
 --target-os=mingw32 --arch=x86_64 --enable-cross-compile \
 --cc=x86_64-w64-mingw32-clang --cxx=x86_64-w64-mingw32-clang++ \
 --ar=llvm-ar --ranlib=llvm-ranlib --nm=llvm-nm --strip=llvm-strip \
 --disable-autodetect --disable-everything --disable-network --disable-doc \
 --disable-debug --disable-x86asm --disable-avdevice --disable-swscale \
 --disable-ffplay --disable-ffprobe --enable-ffmpeg --enable-protocol=file \
 --enable-demuxer=mp3,ogg,wav,flac,aac,mov \
 --enable-parser=mpegaudio,vorbis,flac,aac \
 --enable-decoder=mp3,mp3float,vorbis,flac,aac,pcm_s16le,pcm_u8,pcm_s8,pcm_s24le,pcm_s32le,pcm_f32le,adpcm_ima_wav \
 --enable-encoder=pcm_s16le --enable-muxer=pcm_s16le \
 --enable-demuxer=pcm_s16le --enable-filter=aresample,aformat,anull \
 --disable-iconv --disable-bzlib --disable-zlib --disable-lzma
make -j2
./ffmpeg.exe -version

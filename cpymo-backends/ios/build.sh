#!/usr/bin/env bash
cd FFmpeg-iOS-build-script
./build-ffmpeg.sh arm64

cd ..
cmake -S. -Bbuild -DCMAKE_TOOLCHAIN_FILE=./ios-cmake/ios.toolchain.cmake -DPLATFORM=OS64 -DENABLE_BITCODE=0
cmake --build build -j `sysctl -n hw.physicalcpu` --config Release
cd build
mkdir Payload
mv cpymo.app Payload/
zip -r cpymo.ipa Payload

curl https://codeload.github.com/FFmpeg/FFmpeg/tar.gz/refs/tags/n5.0.1 --output ffmpeg.tar.gz
tar -xf ffmpeg.tar.gz

cd FFmpeg-n5.0.1

chmod +x ./configure

emconfigure ./configure \
		--cc=emcc --cxx=em++ --ar=emar --dep-cc=emcc \
		--objcc=emcc --ranlib=emranlib \
		--nm="emnm -g" \
		--prefix=$(pwd)/../build.FFmpeg.Emscripten/ \
		--enable-cross-compile \
		--disable-shared \
		--disable-runtime-cpudetect \
		--disable-armv5te \
		--disable-programs \
		--disable-asm \
		--disable-inline-asm \
		--disable-doc \
		--disable-network \
		--disable-encoders \
		--disable-filters \
		--disable-indevs \
		--disable-protocols \
		--enable-bsfs \
		--disable-muxers \
		--enable-protocol=file \
		--enable-static \
		--disable-x86asm \
		--arch=x86_32 \
		--disable-decoder=gif,webp,bmp,dds,mjpeg,jpeg1s,jpeg2000,photocd,png,ppm,psd,qpeg,sga,sgi,smvjpeg,text \
		--disable-parser=bmp,gif,mjpeg,webp,png,ipu,jpeg2000,pnm \
		--enable-small \
		--disable-debug \
		--disable-armv6t2 \
		--target-os=none \
		--extra-cflags=" -O2 -ftree-vectorize -fomit-frame-pointer -ffast-math -D_BSD_SOURCE" \
		--extra-cxxflags=" -O2 -ftree-vectorize -fomit-frame-pointer -ffast-math -fno-rtti -fno-exceptions -std=gnu++11 -D_BSD_SOURCE" \
		--disable-bzlib \
		--disable-iconv \
		--disable-lzma \
		--disable-securetransport \
		--disable-xlib \
		--disable-pthreads \
		--enable-gpl \
		--enable-version3 

make install -j

cd ..

rm -rf FFmpeg-n5.0.1 ffmpeg.tar.gz



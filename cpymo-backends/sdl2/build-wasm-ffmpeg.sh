curl https://ffmpeg.org/releases/ffmpeg-5.0.tar.gz --output ffmpeg-5.0.tar.gz
tar -xf ffmpeg-5.0.tar.gz

cd ffmpeg-5.0

chmod +x ./configure

emconfigure ./configure \
		--cc="emcc" --cxx="em++" --ar="emar" \
		--prefix=$(pwd)/../build.FFmpeg.Emscripten/ \
		--enable-cross-compile \
		--disable-shared \
		--disable-runtime-cpudetect \
		--disable-armv5te \
		--disable-programs \
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
		--arch=emscripten \
		--disable-decoder=gif,webp,bmp,dds,mjpeg,jpeg1s,jpeg2000,photocd,png,ppm,psd,qpeg,sga,sgi,smvjpeg,text \
		--disable-parser=bmp,gif,mjpeg,webp,png,ipu,jpeg2000,pnm \
		--enable-small \
		--disable-debug \
		--cpu=generic \
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
rm ffmpeg-5.0.tar.gz -f
rm ffmpeg-5.0 -rf


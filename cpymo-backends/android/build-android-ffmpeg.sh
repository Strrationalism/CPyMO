cd ffmpeg-android-maker
curl https://codeload.github.com/FFmpeg/FFmpeg/tar.gz/refs/tags/n5.0.1 --output ffmpeg.tar.gz
./ffmpeg-android-maker.sh \
    -android=18 \
    --source-tar=ffmpeg.tar.gz
rm -rf ffmpeg.tar.gz
cd ..

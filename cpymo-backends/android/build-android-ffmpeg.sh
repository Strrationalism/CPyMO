export FFMPEG_EXTRA_CONFIGURE_FLAGS="--disable-doc"

cd ffmpeg-android-maker

./ffmpeg-android-maker.sh \
    -android=18 \
    --source-git-tag=n5.1

cd ..

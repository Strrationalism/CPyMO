#!/usr/bin/env bash
# Build a redistributable FFmpeg executable without GPL/nonfree components.
# The EXACT source tarball, config, command log and executable hashes are retained
# for inclusion with the corresponding macOS package. Only used for new RCs.
set -euo pipefail
if [[ "$(uname -s)" != Darwin ]]; then echo 'macOS host required' >&2; exit 1; fi
if (( $# != 1 )); then echo "usage: bash $0 NEW-OUTPUT-DIR" >&2; exit 2; fi
OUT="$1"
if [[ -e "$OUT" ]]; then echo "Output exists: $OUT" >&2; exit 2; fi
mkdir -p "$OUT/bin" "$OUT/source" "$OUT/build"
OUT="$(cd "$OUT" && pwd -P)"
trap 'rc=$?; echo "FFmpeg source build failed; recent diagnostics:" >&2; for log in "$OUT/source/configure.log" "$OUT/source/make.log"; do if [[ -f "$log" ]]; then echo "$log" >&2; tail -n 90 "$log" >&2; fi; done; exit "$rc"' ERR
VERSION="7.1.5"
ARCHIVE="ffmpeg-${VERSION}.tar.xz"
URL="https://ffmpeg.org/releases/${ARCHIVE}"
curl --fail --location --retry 3 --silent --show-error "$URL" -o "$OUT/source/$ARCHIVE"
shasum -a 256 "$OUT/source/$ARCHIVE" > "$OUT/source/SOURCE-SHA256SUMS.txt"
tar -xJf "$OUT/source/$ARCHIVE" -C "$OUT/build"
cd "$OUT/build/ffmpeg-$VERSION"
# Internal decoders, swscale and swresample are included. No external
# nonfree/GPL libraries and no sidecar wrapper binaries are permitted.
CONFIG_ARGS=(
  "--prefix=$OUT/install"
  "--disable-autodetect"
  "--disable-x86asm"
  "--disable-gpl"
  "--disable-nonfree"
  "--disable-doc"
  "--disable-debug"
  "--disable-network"
  "--disable-ffplay"
  "--disable-ffprobe"
  "--enable-ffmpeg"
)
printf '%s\n' "FFmpeg $VERSION" "$URL" "${CONFIG_ARGS[@]}" > "$OUT/source/BUILD-CONFIG.txt"
./configure "${CONFIG_ARGS[@]}" > "$OUT/source/configure.log" 2>&1
make -j"$(sysctl -n hw.ncpu)" ffmpeg > "$OUT/source/make.log" 2>&1
cp ffmpeg "$OUT/bin/ffmpeg"
chmod 755 "$OUT/bin/ffmpeg"
"$OUT/bin/ffmpeg" -version > "$OUT/source/FFMPEG-VERSION.txt" 2>&1
"$OUT/bin/ffmpeg" -buildconf > "$OUT/source/FFMPEG-BUILDCONF.txt" 2>&1
"$OUT/bin/ffmpeg" -L > "$OUT/source/FFMPEG-LICENSE.txt" 2>&1
grep -F -- '--disable-gpl' "$OUT/source/FFMPEG-BUILDCONF.txt"
grep -F -- '--disable-nonfree' "$OUT/source/FFMPEG-BUILDCONF.txt"
if grep -Eiq -- 'nonfree and unredistributable|--enable-nonfree|--enable-gpl' "$OUT/source/FFMPEG-BUILDCONF.txt" "$OUT/source/FFMPEG-LICENSE.txt"; then
  echo 'Rejected nonredistributable or GPL-enabled FFmpeg build' >&2; exit 1
fi
shasum -a 256 "$OUT/bin/ffmpeg" > "$OUT/source/BINARY-SHA256SUMS.txt"
cat > "$OUT/source/README-REDISTRIBUTION.txt" <<'TXT'
This CPyMO macOS converter bundles an executable compiled from the included
unmodified FFmpeg source archive (ffmpeg-7.1.5.tar.xz), with its exact
configure options and build logs. Its build has --disable-gpl and
--disable-nonfree and does not enable external GPL/nonfree libraries.
FFmpeg's LGPL and component notices are included in the original source;
the bundled executable is invoked as a separate process by the MIT converter.
See https://ffmpeg.org/legal.html and FFMPEG-LICENSE.txt.
TXT

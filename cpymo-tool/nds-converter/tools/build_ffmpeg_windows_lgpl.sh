#!/usr/bin/env bash
# Build a self-contained LGPL-only FFmpeg for Windows/MINGW64 in GitHub Actions.
# Bundle the exact corresponding unmodified source archive/configuration.
set -euo pipefail
if (( $# != 1 )); then echo "usage: bash $0 NEW-OUTPUT-DIR" >&2; exit 2; fi
OUT="$1"
if [[ -e "$OUT" ]]; then echo "Output exists: $OUT" >&2; exit 2; fi
mkdir -p "$OUT/bin" "$OUT/source" "$OUT/build"
OUT="$(cd "$OUT" && pwd -P)"
trap 'rc=$?; echo "Windows LGPL FFmpeg build failed; diagnostic logs:" >&2; for log in "$OUT/source/configure.log" "$OUT/source/make.log"; do if [[ -f "$log" ]]; then echo "$log" >&2; tail -n 75 "$log" >&2; fi; done; exit "$rc"' ERR
VERSION="7.1.5"
ARCHIVE="ffmpeg-${VERSION}.tar.xz"
URL="https://ffmpeg.org/releases/${ARCHIVE}"
EXPECTED_SOURCE_SHA256="de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f"
# CI may reuse the original archive bundled by the paired macOS job. Local
# callers can supply the same verified archive without changing the default URL.
if [[ -n "${FFMPEG_SOURCE_ARCHIVE:-}" ]]; then
  if [[ ! -f "$FFMPEG_SOURCE_ARCHIVE" ]]; then
    echo "FFmpeg source archive missing: $FFMPEG_SOURCE_ARCHIVE" >&2; exit 1
  fi
  cp "$FFMPEG_SOURCE_ARCHIVE" "$OUT/source/$ARCHIVE"
else
  curl --fail --location --retry 3 --retry-all-errors \
    --connect-timeout 20 --max-time 180 --retry-max-time 600 \
    --silent --show-error "$URL" -o "$OUT/source/$ARCHIVE"
fi
printf '%s  %s\n' "$EXPECTED_SOURCE_SHA256" "$OUT/source/$ARCHIVE" | sha256sum --check --status
sha256sum "$OUT/source/$ARCHIVE" > "$OUT/source/SOURCE-SHA256SUMS.txt"
tar -xJf "$OUT/source/$ARCHIVE" -C "$OUT/build"
cd "$OUT/build/ffmpeg-$VERSION"
CONFIG_ARGS=(
 "--prefix=$OUT/install"
 "--target-os=mingw32"
 "--arch=x86_64"
 "--cc=gcc"
 "--disable-x86asm"
 "--disable-autodetect"
 "--disable-gpl"
 "--disable-nonfree"
 "--disable-doc"
 "--disable-debug"
 "--disable-network"
 "--disable-ffplay"
 "--disable-ffprobe"
 "--disable-shared"
 "--enable-static"
 "--enable-ffmpeg"
 "--extra-ldflags=-static"
)
printf '%s\n' "FFmpeg $VERSION" "$URL" "${CONFIG_ARGS[@]}" > "$OUT/source/BUILD-CONFIG.txt"
./configure "${CONFIG_ARGS[@]}" > "$OUT/source/configure.log" 2>&1
make -j4 ffmpeg.exe > "$OUT/source/make.log" 2>&1
cp ffmpeg.exe "$OUT/bin/ffmpeg.exe"
"$OUT/bin/ffmpeg.exe" -version > "$OUT/source/FFMPEG-VERSION.txt" 2>&1
"$OUT/bin/ffmpeg.exe" -buildconf > "$OUT/source/FFMPEG-BUILDCONF.txt" 2>&1
"$OUT/bin/ffmpeg.exe" -L > "$OUT/source/FFMPEG-LICENSE.txt" 2>&1
grep -F -- '--disable-gpl' "$OUT/source/FFMPEG-BUILDCONF.txt"
grep -F -- '--disable-nonfree' "$OUT/source/FFMPEG-BUILDCONF.txt"
if grep -Eiq -- 'nonfree and unredistributable|--enable-nonfree|--enable-gpl' "$OUT/source/FFMPEG-BUILDCONF.txt" "$OUT/source/FFMPEG-LICENSE.txt"; then
  echo 'Rejected GPL/nonfree FFmpeg build' >&2; exit 1
fi
sha256sum "$OUT/bin/ffmpeg.exe" > "$OUT/source/BINARY-SHA256SUMS.txt"
cat > "$OUT/source/README-REDISTRIBUTION.txt" <<'TXT'
FFmpeg 7.1.5 has been built from the included original source tarball using
the recorded configure flags; no external GPL/nonfree libraries are enabled.
The corresponding source, build configuration, binary hash, and license text
accompany the separate FFmpeg executable included in the Windows converter.
See https://ffmpeg.org/legal.html. This does not license CPyMO game content.
TXT

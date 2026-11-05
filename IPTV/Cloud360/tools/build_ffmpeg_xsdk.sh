#!/usr/bin/env bash
set -eo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$FFMPEG_SOURCE"
TOOLCHAIN="$XSDK_TOOLCHAIN"
DEST="$CLOUD360_FFMPEG_ROOT"
BUILD="$FFMPEG_BUILD_DIR"
if [ -z "$TOOLCHAIN" ]; then TOOLCHAIN="$HOME/xsdk-gcc"; fi
if [ -z "$DEST" ]; then DEST="$ROOT/deps/ffmpeg-xsdk"; fi
if [ -z "$BUILD" ]; then BUILD="$ROOT/deps/ffmpeg-xsdk-build"; fi
if [ -z "$SOURCE" ] || [ ! -f "$SOURCE/configure" ]; then
  echo "Set FFMPEG_SOURCE to a verified FFmpeg source checkout." >&2; exit 2
fi
if [ ! -x "$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" ]; then
  echo "Missing powerpc-xsdk-elf-gcc. Build the XSDK toolchain first." >&2; exit 2
fi
SOURCE="$(realpath "$SOURCE")"
mkdir -p "$BUILD" "$DEST"
BUILD="$(realpath "$BUILD")"
DEST="$(realpath "$DEST")"
cd "$BUILD"
# Experimental PPC/newlib port; no assertion that FFmpeg cross-compiles unchanged.
"$SOURCE/configure" \
  --prefix="$DEST" --target-os=none --arch=powerpc --cpu=cell \
  --enable-cross-compile --cross-prefix="$TOOLCHAIN/bin/powerpc-xsdk-elf-" \
  --cc="$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" \
  --disable-shared --enable-static --enable-small \
  --disable-programs --disable-doc \
  --disable-everything --disable-autodetect \
  --disable-avdevice --disable-avfilter --disable-avformat \
  --disable-network --disable-postproc --disable-swresample --disable-swscale \
  --disable-pthreads --disable-w32threads --disable-os2threads \
  --disable-asm --disable-runtime-cpudetect --disable-iconv \
  --disable-bzlib --disable-zlib \
  --enable-decoder=h264,mpeg2video,aac,mp3,ac3 \
  --enable-parser=h264,mpegvideo,aac,mpegaudio,ac3 \
  --extra-cflags="-mcpu=cell -fno-pic -O2 -fno-strict-aliasing"
make -j2
make install
test -s "$DEST/lib/libavcodec.a"
test -s "$DEST/lib/libavutil.a"
echo "FFmpeg static libraries installed in $DEST"

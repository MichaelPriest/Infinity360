#!/usr/bin/env bash
set -eo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="$XSDK_ROOT"
TOOLCHAIN="$XSDK_TOOLCHAIN"
LOGOS="$CLOUD360_WITH_LOGOS"
DECODER="$CLOUD360_WITH_FFMPEG"
FFROOT="$CLOUD360_FFMPEG_ROOT"
if [ -z "$SDK" ]; then SDK="$HOME/xsdk"; fi
if [ -z "$TOOLCHAIN" ]; then TOOLCHAIN="$HOME/xsdk-gcc"; fi
if [ -z "$LOGOS" ]; then LOGOS="OFF"; fi
if [ -z "$DECODER" ]; then DECODER="OFF"; fi
if [ -z "$FFROOT" ]; then FFROOT="$ROOT/deps/ffmpeg-xsdk"; fi
if [ ! -f "$SDK/cmake/xsdk.cmake" ] || [ ! -s "$SDK/build/libxsdk.a" ]; then
  echo "XSDK missing. See xenosworks/xsdk docs/GETTING_STARTED.md" >&2; exit 2
fi
if [ ! -x "$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" ]; then
  echo "PowerPC XSDK compiler not found at $TOOLCHAIN" >&2; exit 2
fi
cmake -S "$ROOT/native" -B "$ROOT/native/build" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$SDK/cmake/xsdk.cmake" \
  -DXSDK_TOOLCHAIN="$TOOLCHAIN" -DCLOUD360_WITH_LOGOS="$LOGOS" \
  -DCLOUD360_WITH_FFMPEG="$DECODER" -DCLOUD360_FFMPEG_ROOT="$FFROOT" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/native/build" --parallel
XEX="$ROOT/native/build/cloud360.xex"
if [ ! -s "$XEX" ]; then echo "Missing actual compiled XEX" >&2;exit 1;fi
if [ "$DECODER" = "ON" ]; then
  command -v ffmpeg >/dev/null || { echo "ffmpeg CLI needed for test fixture" >&2;exit 2; }
  ffmpeg -hide_banner -loglevel error -y -f lavfi -i "testsrc2=size=320x180:rate=8" \
    -frames:v 8 -c:v libx264 -profile:v baseline -pix_fmt yuv420p \
    -preset ultrafast -threads 1 -f h264 "$ROOT/native/build/sample.h264"
  python3 "$ROOT/tools/package_xex.py" --xex "$XEX" \
    --output "$ROOT/Cloud360-Xbox360-smoke-v0.5.zip" \
    --smoke-video "$ROOT/native/build/sample.h264"
else
  python3 "$ROOT/tools/package_xex.py" --xex "$XEX" \
    --output "$ROOT/Cloud360-Xbox360-smoke-v0.5.zip"
fi
echo "XEX packaging complete; hardware testing still required."

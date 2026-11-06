#!/usr/bin/env bash
# Cloud360 SAFE diagnostic v0.5.2: no logo shaders or codec libraries.
set -eo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="$XSDK_ROOT"
TOOLCHAIN="$XSDK_TOOLCHAIN"
if [ -z "$SDK" ]; then SDK="$HOME/xsdk"; fi
if [ -z "$TOOLCHAIN" ]; then TOOLCHAIN="$HOME/xsdk-gcc"; fi
if [ ! -f "$SDK/cmake/xsdk.cmake" ] || [ ! -s "$SDK/build/libxsdk.a" ]; then
  echo "Missing XSDK or built libxsdk.a" >&2; exit 2
fi
if [ ! -x "$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" ]; then
  echo "Missing XSDK PowerPC toolchain" >&2; exit 2
fi
cmake -S "$ROOT/native" -B "$ROOT/native/build" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$SDK/cmake/xsdk.cmake" \
  -DXSDK_TOOLCHAIN="$TOOLCHAIN" \
  -DCLOUD360_WITH_LOGOS=OFF -DCLOUD360_WITH_FFMPEG=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/native/build" --parallel
python3 "$ROOT/tools/package_xex.py" \
  --xex "$ROOT/native/build/cloud360.xex" \
  --probe-xex "$ROOT/native/build/cloud360_probe.xex" \
  --output "$ROOT/Cloud360-Xbox360-SAFE-v0.5.2.zip"
echo "Compiled binaries packaged. Console safety NOT YET verified."

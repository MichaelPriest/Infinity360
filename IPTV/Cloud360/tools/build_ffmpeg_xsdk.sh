#!/usr/bin/env bash
set -eo pipefail
export LC_ALL=C
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$FFMPEG_SOURCE"
TOOLCHAIN="$XSDK_TOOLCHAIN"
SDK="${XSDK_ROOT:-$HOME/xsdk}"
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
if [ ! -s "$SDK/build/libxsdk.a" ] || [ ! -f "$SDK/src/xsdk.ld" ]; then
  echo "Build XSDK support library first (XSDK_ROOT)." >&2; exit 2
fi
SDK="$(realpath "$SDK")"
TOOLCHAIN="$(realpath "$TOOLCHAIN")"
SOURCE="$(realpath "$SOURCE")"
mkdir -p "$BUILD" "$DEST"
BUILD="$(realpath "$BUILD")"
DEST="$(realpath "$DEST")"
cd "$BUILD"
# Match the reviewed XSDK CMake link rule for configure feature probes.
# Do not force HAVE_ERF/HAVE_HYPOT: prove both resolve in the target libm.
cat > xsdk-math-probe.c <<'EOF'
#include <math.h>
static double (*volatile e)(double)=erf;
static double (*volatile h)(double,double)=hypot;
int main(void){volatile double x=0.25;return e(x)+h(x,x)>0.0?0:1;}
EOF
"$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" -mcpu=cell -fno-pic -fno-builtin \
  -static -nostdlib -T "$SDK/src/xsdk.ld" xsdk-math-probe.c -o xsdk-math-probe.elf \
  -Wl,--start-group -lstdc++ "$SDK/build/libxsdk.a" -lc -lm -lgcc -Wl,--end-group
test -s xsdk-math-probe.elf
# Cross-compile only: never execute a PPC binary on the build host.
# Use the SDK startup/linker rather than the default bare newlib CRT.
"$SOURCE/configure" \
  --prefix="$DEST" --target-os=none --arch=powerpc --cpu=cell \
  --enable-cross-compile --cross-prefix="$TOOLCHAIN/bin/powerpc-xsdk-elf-" \
  --cc="$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" \
  --ld="$TOOLCHAIN/bin/powerpc-xsdk-elf-gcc" \
  --extra-ldflags="-mcpu=cell -static -nostdlib -T $SDK/src/xsdk.ld" \
  --extra-libs="-Wl,--start-group -lstdc++ $SDK/build/libxsdk.a -lc -lm -lgcc -Wl,--end-group" \
  --pkg-config=false \
  --disable-shared --enable-static --enable-small \
  --disable-programs --disable-doc \
  --disable-everything --disable-autodetect \
  --disable-avdevice --disable-avfilter --enable-avformat --enable-demuxer=mov,mpegts,mpegps \
  --disable-network --disable-postproc --disable-swresample --disable-swscale \
  --disable-gpl --disable-nonfree \
  --disable-pthreads --disable-w32threads --disable-os2threads \
  --disable-asm --disable-runtime-cpudetect --disable-iconv \
  --disable-bzlib --disable-zlib \
  --enable-decoder=h264,mpeg1video,mpeg2video,mpeg4,aac,mp2,mp3,ac3,eac3 \
  --enable-parser=h264,mpegvideo,mpeg4video,aac,mpegaudio,ac3 \
  --extra-cflags="-mcpu=cell -fno-pic -O2 -fno-strict-aliasing -I$SDK/include"
grep -qx "#define HAVE_ERF 1" config.h
grep -qx "#define HAVE_HYPOT 1" config.h
make -j2
make install
test -s "$DEST/lib/libavformat.a"
test -s "$DEST/lib/libavcodec.a"
test -s "$DEST/lib/libavutil.a"
for lib in avformat avcodec avutil; do
  "$TOOLCHAIN/bin/powerpc-xsdk-elf-ar" t "$DEST/lib/lib$lib.a" > "$BUILD/$lib-objects.txt"
  test -s "$BUILD/$lib-objects.txt"
done
echo "FFmpeg static libraries installed in $DEST"

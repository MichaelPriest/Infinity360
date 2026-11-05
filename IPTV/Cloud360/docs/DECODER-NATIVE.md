# Cloud360 — real FFmpeg software decoder v0.5.1

Software decoder source code was integrated:
- core/ffmpeg_decoder.c implements avcodec_send_packet and avcodec_receive_frame.
- core/offline_decode.c decodes raw H.264, MPEG-2, AAC/ADTS, MP3 and AC-3 files,
  calling libavcodec and recording the number of frames, samples and dimensions.
- native/main.c: CONFIG tab, press A to try decoding game:\sample.h264
  when CLOUD360_WITH_FFMPEG is ON.
- native/CMakeLists.txt links static cross-built PowerPC XSDK libavcodec/libavutil.
- tests/test_offline_decode.c exercises actual FFmpeg decode on the host.
- tools/build_ffmpeg_xsdk.sh is an EXPERIMENTAL cross-build script.

## Procedure (WSL 2 or Linux)

Follow xenosworks/xsdk docs/GETTING_STARTED.md to build XSDK and its compiler.
Obtain a verified upstream FFmpeg source tree (not an x86 compiled library).
From IPTV/Cloud360, run:

    export FFMPEG_SOURCE="$HOME/src/FFmpeg"
    bash tools/build_ffmpeg_xsdk.sh
    CLOUD360_WITH_FFMPEG=ON bash tools/build_xex.sh

The port can fail on newlib-specific symbols or linker flags: no native XEX
with FFmpeg has been compiled or tested on an Xbox 360 yet. A PC test is NOT
a native playback test. A build with CLOUD360_WITH_FFMPEG=OFF keeps the
diagnostic interface usable and skips FFmpeg entirely.

## First console smoke test
Extract the generated ZIP in Hdd1:/Homebrew/Cloud360-Test and start
default.xex with Aurora. In the CONFIG tab, press A: the app reads
sample.h264 and decodes it as elementary Annex B H.264.
Look for cloud360: H264: rc=0 frames=... resolution=... in the XSDK log.

This does not display the decoded frame yet: GPU RGB/YUV presentation,
audio output, network HLS/MPEG-TS, A/V timestamps and buffer are pending.

## Licensing
FFmpeg licensing (LGPL or GPL) depends on build options. Static
distribution of linked FFmpeg requires respecting the applicable
relinking/source/notice requirements. No Orbita or proprietary XDK binaries
are incorporated here. Test only streams with authorized external access.

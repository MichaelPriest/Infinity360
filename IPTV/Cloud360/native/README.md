# Xbox 360 build - Cloud360 IPTV
This is an experimental Xbox 360 homebrew interface, not a fully working IPTV player.

Requirements: XSDK GCC toolchain and CMake/Ninja in WSL or Linux.
Review the current instructions in https://github.com/xenosworks/xsdk before building.

From IPTV/Cloud360/native:
  cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HOME/xsdk/cmake/xsdk.cmake -DXSDK_TOOLCHAIN=$HOME/xsdk-gcc -DCMAKE_BUILD_TYPE=Release
  cmake --build build

If successful, build/cloud360.xex can be renamed default.xex.
Install default.xex plus a licensed playlist.m3u in the same app folder.
Run it manually from Aurora or Freestyle on a console you are authorized to use.

Not implemented: HTTP/TLS console downloads, video decoding/playback, DRM, stream transcoding,
native Xtream login, EPG/XMLTV, artwork. No XEX compilation or console execution has yet
been verified in this repository. Never include your playlist or account credentials in git.

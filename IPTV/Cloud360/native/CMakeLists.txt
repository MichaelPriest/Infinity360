# Cloud360 Xbox 360 executable via xenosworks/xsdk.
cmake_minimum_required(VERSION 3.25)
project(cloud360 LANGUAGES C)
option(CLOUD360_WITH_LOGOS "Build offline GPU logo renderer (needs glslang-xenon)" OFF)
option(CLOUD360_WITH_FFMPEG "Link actual FFmpeg software decode from a cross-built static SDK" OFF)
set(CLOUD360_FFMPEG_ROOT "" CACHE PATH "Installed FFmpeg prefix for powerpc-xsdk-elf")

add_executable(cloud360 main.c ../core/cloud360.c)
target_include_directories(cloud360 PRIVATE ../core)
set_target_properties(cloud360 PROPERTIES C_STANDARD 17 C_EXTENSIONS ON)
target_compile_options(cloud360 PRIVATE -Wall -Wextra)

if(CLOUD360_WITH_LOGOS)
  target_sources(cloud360 PRIVATE logo.c)
  xsdk_add_shaders(cloud360 logo.vert logo.frag)
else()
  target_sources(cloud360 PRIVATE logo_stub.c)
endif()

if(CLOUD360_WITH_FFMPEG)
  if(NOT EXISTS "${CLOUD360_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
    message(FATAL_ERROR "Missing FFmpeg includes at CLOUD360_FFMPEG_ROOT. Run tools/build_ffmpeg_xsdk.sh first.")
  endif()
  if(NOT EXISTS "${CLOUD360_FFMPEG_ROOT}/lib/libavcodec.a" OR
     NOT EXISTS "${CLOUD360_FFMPEG_ROOT}/lib/libavutil.a")
    message(FATAL_ERROR "Missing static libavcodec.a/libavutil.a in CLOUD360_FFMPEG_ROOT")
  endif()
  target_compile_definitions(cloud360 PRIVATE CLOUD360_WITH_FFMPEG=1)
  target_sources(cloud360 PRIVATE
      ../core/codec_probe.c
      ../core/ffmpeg_decoder.c
      ../core/offline_decode.c)
  target_include_directories(cloud360 PRIVATE "${CLOUD360_FFMPEG_ROOT}/include")
  target_link_libraries(cloud360 PRIVATE
      "${CLOUD360_FFMPEG_ROOT}/lib/libavcodec.a"
      "${CLOUD360_FFMPEG_ROOT}/lib/libavutil.a")
endif()
xsdk_add_xex(cloud360 TITLE_ID 0x43333630 NAME "Cloud360 IPTV" VERSION 0.5.1.0)

cmake_minimum_required(VERSION 3.25)
project(cloud360 LANGUAGES C CXX)
# GPU-safe diagnostic is deliberately isolated from FFmpeg, textures and logos.
option(CLOUD360_WITH_LOGOS "ENABLE only after display restoration validated on hardware" OFF)
option(CLOUD360_WITH_FFMPEG "ENABLE only after display restoration validated on hardware" OFF)
if(CLOUD360_WITH_LOGOS OR CLOUD360_WITH_FFMPEG)
  message(FATAL_ERROR "v0.5.2 diagnostics must be built with LOGOS=OFF and FFMPEG=OFF")
endif()
add_executable(cloud360 main.c ../core/cloud360.c)
set_target_properties(cloud360 PROPERTIES C_STANDARD 17 C_EXTENSIONS ON)
target_include_directories(cloud360 PRIVATE ../core)
target_compile_options(cloud360 PRIVATE -Wall -Wextra -Werror)
xsdk_add_xex(cloud360 TITLE_ID 0x43333630 NAME "Cloud360 SAFE v0.5.2" VERSION 0.5.2.0)

# Run FIRST, before opening any app that takes over the GPU.
add_executable(cloud360_probe probe.c)
set_target_properties(cloud360_probe PROPERTIES C_STANDARD 17 C_EXTENSIONS ON)
target_compile_options(cloud360_probe PRIVATE -Wall -Wextra -Werror)
xsdk_add_xex(cloud360_probe TITLE_ID 0x43333631 NAME "Cloud360 GPU Probe" VERSION 0.5.2.0)

# AuroraX visual navigation test, separate from Cloud360 diagnostics.
add_executable(aurorax_alpha aurorax_alpha.c ../core/cloud360.c)
set_target_properties(aurorax_alpha PROPERTIES C_STANDARD 17 C_EXTENSIONS ON)
target_include_directories(aurorax_alpha PRIVATE ../core)
target_compile_options(aurorax_alpha PRIVATE -Wall -Wextra -Werror)
xsdk_add_xex(aurorax_alpha TITLE_ID 0x41555831 NAME "INFINITY 360 Alpha GPU Test" VERSION 0.1.0.0)

# Compile and link production C++ code on the open PowerPC toolchain before
# touching the GPU. This is not the full dashboard or an XDK replacement.
set(INFINITY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
add_executable(infinity_core_probe infinity_core_probe.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/HomeCanvas.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/MediaUrl.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/TvCatalog.cpp
  ${INFINITY_ROOT}/Source/GameScanner.cpp ../core/cloud360.c)
target_include_directories(infinity_core_probe PRIVATE
  ${INFINITY_ROOT}/Platform/Xbox360 ${INFINITY_ROOT}/Source ../core)
set_target_properties(infinity_core_probe PROPERTIES C_STANDARD 17 CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(infinity_core_probe PRIVATE -Wall -Wextra)
xsdk_add_xex(infinity_core_probe TITLE_ID 0x494E4631 NAME "INFINITY 360 Core Probe" VERSION 0.1.1.0)

# Experimental production pixel presenter; separate binary, never replaces the
# no-GPU and text-only diagnostics or the integrated XDK build.
add_executable(infinity_canvas_probe infinity_canvas_probe.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/HomeCanvas.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/TvCatalog.cpp
  ${INFINITY_ROOT}/Source/GameScanner.cpp ../core/cloud360.c)
target_include_directories(infinity_canvas_probe PRIVATE
  ${INFINITY_ROOT}/Platform/Xbox360 ${INFINITY_ROOT}/Source ../core)
set_target_properties(infinity_canvas_probe PROPERTIES C_STANDARD 17 CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
xsdk_add_shaders(infinity_canvas_probe infinity_canvas.vert infinity_canvas.frag)
xsdk_add_xex(infinity_canvas_probe TITLE_ID 0x494E4632 NAME "INFINITY 360 Canvas Probe" VERSION 0.1.2.0)

# Full production menu linked to the same Xenos presenter. File operations
# are opt-in in this isolated build; optional XEX launch and LAN FTP read-only, no media adapter.
add_executable(infinity_menu_preview infinity_canvas_probe.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/FileWorkspace.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/ConsoleHome.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/HomeCanvas.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/StoreCatalog.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/UnityCatalog.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/ArtImport.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/ContentBackup.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/ContentInstall.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/PackageImport.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/MpegPlayback.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/CollectionStore.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/StorageInfo.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/NetworkInfo.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/FtpServer.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/FtpSession.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/TvCatalog.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/TvGuide.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/CloudTv.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/MediaUrl.cpp
  ${INFINITY_ROOT}/Platform/Xbox360/third_party/yxml/yxml.c
  ${INFINITY_ROOT}/Source/GameScanner.cpp
  ${INFINITY_ROOT}/Source/ContentCatalog.cpp
  ${INFINITY_ROOT}/Source/PluginRegistry.cpp
  ${INFINITY_ROOT}/Source/TrainerCatalog.cpp ../core/cloud360.c)
target_compile_definitions(infinity_menu_preview PRIVATE INFINITY_FULL_MENU_PREVIEW=1)
target_include_directories(infinity_menu_preview PRIVATE
  ${INFINITY_ROOT}/Platform/Xbox360 ${INFINITY_ROOT}/Source ../core)
set_target_properties(infinity_menu_preview PROPERTIES C_STANDARD 17 CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
xsdk_add_shaders(infinity_menu_preview infinity_canvas.vert infinity_canvas.frag)
# Optional open-source XSDK TLS stack. No fallback to unverified HTTP.
if(DEFINED MBEDTLS_XENON_DIR AND DEFINED CURL_XENON_DIR)
  set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory("${MBEDTLS_XENON_DIR}" mbedtls)
  set(MBEDTLS_INCLUDE_DIR "${MBEDTLS_XENON_DIR}/include" CACHE PATH "")
  set(MBEDTLS_LIBRARY mbedtls CACHE STRING "")
  set(MBEDX509_LIBRARY mbedx509 CACHE STRING "")
  set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "")
  set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  add_subdirectory("${CURL_XENON_DIR}" curl)
  target_sources(infinity_menu_preview PRIVATE ${INFINITY_ROOT}/Platform/Xbox360/UnityStage.cpp XsdkDownload.cpp ${INFINITY_ROOT}/Platform/Xbox360/StorePackage.cpp ${INFINITY_ROOT}/Platform/Xbox360/DiscInstaller.cpp
    ${INFINITY_ROOT}/Platform/Xbox360/DiscGod.cpp ${INFINITY_ROOT}/Platform/Xbox360/DiscImage.cpp)
  target_link_libraries(infinity_menu_preview PRIVATE CURL::libcurl mbedtls)
  target_compile_definitions(infinity_menu_preview PRIVATE INFINITY_XSDK_HTTPS=1 INFINITY_DISC_INSTALL=1)
endif()
if(DEFINED INFINITY_MINIZ_DIR AND DEFINED MBEDTLS_XENON_DIR AND DEFINED CURL_XENON_DIR)
  include("${INFINITY_ROOT}/Platform/Xbox360/third_party/MinizReader.cmake")
  infinity_miniz(infinity_zip_reader "${INFINITY_MINIZ_DIR}")
  target_sources(infinity_menu_preview PRIVATE ${INFINITY_ROOT}/Platform/Xbox360/StoreZipInstaller.cpp)
  target_link_libraries(infinity_menu_preview PRIVATE infinity_zip_reader)
  target_compile_definitions(infinity_menu_preview PRIVATE INFINITY_STORE_ZIP=1)
endif()
xsdk_add_xex(infinity_menu_preview TITLE_ID 0x494E4633 NAME "INFINITY 360 Menu Preview" VERSION 0.2.10.0)

add_executable(infinity_launch_target infinity_launch_target.c)
set_target_properties(infinity_launch_target PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED ON)
xsdk_add_xex(infinity_launch_target TITLE_ID 0x494E4634 NAME "INFINITY 360 Launch Test" VERSION 0.1.5.0)

# Independent opt-in diagnostic: do not link FFmpeg into the production menu.
if(DEFINED CLOUD360_CONTAINER_FFMPEG_ROOT)
  set(FFROOT "${CLOUD360_CONTAINER_FFMPEG_ROOT}")
  foreach(lib avformat avcodec avutil)
    if(NOT EXISTS "${FFROOT}/lib/lib${lib}.a")
      message(FATAL_ERROR "Missing experimental FFmpeg static library: ${lib}")
    endif()
  endforeach()
  add_executable(cloud360_container_probe container_probe.c ../core/container_decode.c)
  target_include_directories(cloud360_container_probe PRIVATE ../core "${FFROOT}/include")
  target_link_libraries(cloud360_container_probe PRIVATE "${FFROOT}/lib/libavformat.a" "${FFROOT}/lib/libavcodec.a" "${FFROOT}/lib/libavutil.a" m)
  xsdk_add_xex(cloud360_container_probe TITLE_ID 0x43333632 NAME "Cloud360 Container Probe" VERSION 0.5.3.0)
  add_executable(cloud360_av_probe infinity_canvas_probe.cpp ../core/container_decode.c ../core/frame_output.c
    ${INFINITY_ROOT}/Platform/Xbox360/HomeCanvas.cpp)
  target_compile_definitions(cloud360_av_probe PRIVATE INFINITY_CONTAINER_OUTPUT_PROBE=1)
  target_include_directories(cloud360_av_probe PRIVATE ../core "${FFROOT}/include" ${INFINITY_ROOT}/Platform/Xbox360 ${INFINITY_ROOT}/Source)
  target_link_libraries(cloud360_av_probe PRIVATE "${FFROOT}/lib/libavformat.a" "${FFROOT}/lib/libavcodec.a" "${FFROOT}/lib/libavutil.a" m)
  set_target_properties(cloud360_av_probe PROPERTIES C_STANDARD 17 CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  xsdk_add_shaders(cloud360_av_probe infinity_canvas.vert infinity_canvas.frag)
  xsdk_add_xex(cloud360_av_probe TITLE_ID 0x43333633 NAME "Cloud360 AV Output Probe" VERSION 0.5.4.0)
endif()

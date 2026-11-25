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

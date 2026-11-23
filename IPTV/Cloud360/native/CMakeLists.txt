cmake_minimum_required(VERSION 3.25)
project(cloud360 LANGUAGES C)
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
xsdk_add_xex(aurorax_alpha TITLE_ID 0x41555831 NAME "AuroraX Alpha GPU Test" VERSION 0.1.0.0)

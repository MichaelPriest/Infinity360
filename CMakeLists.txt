cmake_minimum_required(VERSION 3.16)
project(AuroraX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(aurorax_core Source/Dashboard.cpp Source/GameScanner.cpp Source/Input.cpp Engine/Tile.cpp Engine/UIManager.cpp)
target_include_directories(aurorax_core PUBLIC Source Engine)
target_compile_options(aurorax_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(aurorax Source/main.cpp)
target_link_libraries(aurorax PRIVATE aurorax_core)

enable_testing()
add_executable(aurorax_tests Tests/DashboardTests.cpp)
target_link_libraries(aurorax_tests PRIVATE aurorax_core)
add_test(NAME dashboard COMMAND aurorax_tests)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME web_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/WebSmokeTest.py)
endif()

# Exercise the same menu, artwork loader and M3U code used by the XDK presenter.
add_executable(console_home_tests Tests/ConsoleHomeTests.cpp Platform/Xbox360/ConsoleHome.cpp
    Platform/Xbox360/HomeCanvas.cpp Platform/Xbox360/TvCatalog.cpp Platform/Xbox360/TvGuide.cpp
    Platform/Xbox360/CloudTv.cpp Platform/Xbox360/MediaUrl.cpp Platform/Xbox360/third_party/yxml/yxml.c IPTV/Cloud360/core/cloud360.c)
target_include_directories(console_home_tests PRIVATE Platform/Xbox360 IPTV/Cloud360/core)
target_link_libraries(console_home_tests PRIVATE aurorax_core)
add_test(NAME console_home COMMAND console_home_tests ${CMAKE_SOURCE_DIR}/Platform/Xbox360/Assets/home.tga)

# End-to-end media test. The host supplies bytes over HTTP to the console decoder.
find_program(FFMPEG_PROGRAM ffmpeg)
if(Python3_Interpreter_FOUND AND FFMPEG_PROGRAM)
    add_executable(mpeg_playback_probe Tests/MpegPlaybackProbe.cpp Platform/Xbox360/MpegPlayback.cpp)
    target_include_directories(mpeg_playback_probe PRIVATE Platform/Xbox360)
    add_test(NAME stream_playback COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/StreamPlaybackTest.py
        $<TARGET_FILE:mpeg_playback_probe> ${FFMPEG_PROGRAM})
    set_tests_properties(stream_playback PROPERTIES TIMEOUT 70)
endif()

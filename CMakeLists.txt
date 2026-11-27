cmake_minimum_required(VERSION 3.16)
project(AuroraX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(aurorax_core Source/Dashboard.cpp Source/GameScanner.cpp Source/ContentCatalog.cpp Source/PluginRegistry.cpp Source/TrainerCatalog.cpp Source/Input.cpp Engine/Tile.cpp Engine/UIManager.cpp)
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
    add_test(NAME aurora_import COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/AuroraImportTests.py)
    add_test(NAME store_download COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/StoreDownloadTests.py)
    add_test(NAME nexus_import COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/NexusImportTests.py)
endif()

# Exercise the same menu, artwork loader and M3U code used by the XDK presenter.
add_executable(console_home_tests Tests/ConsoleHomeTests.cpp Platform/Xbox360/ConsoleHome.cpp
    Platform/Xbox360/HomeCanvas.cpp Platform/Xbox360/ArtImport.cpp Platform/Xbox360/ContentBackup.cpp Platform/Xbox360/ContentInstall.cpp Platform/Xbox360/PackageImport.cpp Platform/Xbox360/UnityCatalog.cpp Platform/Xbox360/StoreCatalog.cpp Platform/Xbox360/CollectionStore.cpp Platform/Xbox360/FileWorkspace.cpp Platform/Xbox360/StorageInfo.cpp Platform/Xbox360/NetworkInfo.cpp Platform/Xbox360/FtpSession.cpp Platform/Xbox360/FtpServer.cpp Platform/Xbox360/TvCatalog.cpp Platform/Xbox360/TvGuide.cpp
    Platform/Xbox360/CloudTv.cpp Platform/Xbox360/MediaUrl.cpp Platform/Xbox360/third_party/yxml/yxml.c IPTV/Cloud360/core/cloud360.c)
target_include_directories(console_home_tests PRIVATE Platform/Xbox360 IPTV/Cloud360/core)
target_link_libraries(console_home_tests PRIVATE aurorax_core)
include(cmake/InfinityLua.cmake)
infinity_enable_lua(console_home_tests)
if(INFINITY_ENABLE_LUA)
  add_executable(lua_runtime_tests Tests/LuaRuntimeTests.cpp)
  target_include_directories(lua_runtime_tests PRIVATE Platform/Xbox360)
  infinity_enable_lua(lua_runtime_tests)
  add_test(NAME lua_runtime COMMAND lua_runtime_tests ${CMAKE_SOURCE_DIR}/Platform/Xbox360/ReleaseDefaults/Scripts/Subtitles)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(console_home_tests PRIVATE INFINITY_DISC_INSTALL=1)
endif()
add_test(NAME console_home COMMAND console_home_tests ${CMAKE_SOURCE_DIR}/Platform/Xbox360/Assets/home.tga)

# End-to-end media test. The host supplies bytes over HTTP to the console decoder.
find_program(FFMPEG_PROGRAM ffmpeg)
if(Python3_Interpreter_FOUND AND FFMPEG_PROGRAM)
    add_executable(mpeg_playback_probe Tests/MpegPlaybackProbe.cpp Platform/Xbox360/MpegPlayback.cpp)
    target_include_directories(mpeg_playback_probe PRIVATE Platform/Xbox360)
    add_test(NAME stream_playback COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/StreamPlaybackTest.py
        $<TARGET_FILE:mpeg_playback_probe> ${FFMPEG_PROGRAM})
    set_tests_properties(stream_playback PROPERTIES TIMEOUT 180)
endif()

# Manifest scanner is compiled for both PC tests and Xbox 360 target.
add_executable(plugin_registry_tests Tests/PluginRegistryTests.cpp)
target_link_libraries(plugin_registry_tests PRIVATE aurorax_core)
add_test(NAME plugins COMMAND plugin_registry_tests)

# Series360 parity: configurable scan paths for extracted XEX libraries.
add_executable(scan_paths_tests Tests/ScanPathsTests.cpp)
target_link_libraries(scan_paths_tests PRIVATE aurorax_core)
add_test(NAME scan_paths COMMAND scan_paths_tests)

# Read-only Xbox Content/STFS header discovery. Does not launch GOD packages.
add_executable(content_catalog_tests Tests/ContentCatalogTests.cpp)
target_link_libraries(content_catalog_tests PRIVATE aurorax_core)
add_test(NAME content_catalog COMMAND content_catalog_tests)

# Local collections are standalone and use the same FATX-safe store as the UI.
add_executable(collection_store_tests Tests/CollectionStoreTests.cpp Platform/Xbox360/CollectionStore.cpp)
target_include_directories(collection_store_tests PRIVATE Platform/Xbox360)
add_test(NAME collections COMMAND collection_store_tests)

add_executable(file_workspace_tests Tests/FileWorkspaceTests.cpp Platform/Xbox360/FileWorkspace.cpp)
target_include_directories(file_workspace_tests PRIVATE Platform/Xbox360)
add_test(NAME file_workspace COMMAND file_workspace_tests)

add_executable(storage_info_tests Tests/StorageInfoTests.cpp Platform/Xbox360/StorageInfo.cpp)
target_include_directories(storage_info_tests PRIVATE Platform/Xbox360)
add_test(NAME storage_info COMMAND storage_info_tests)

add_executable(network_info_tests Tests/NetworkInfoTests.cpp Platform/Xbox360/NetworkInfo.cpp)
target_include_directories(network_info_tests PRIVATE Platform/Xbox360)
add_test(NAME network_info COMMAND network_info_tests)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)
    add_executable(lan_peer_loopback_tests Tests/LanPeerLoopbackTests.cpp)
    target_include_directories(lan_peer_loopback_tests PRIVATE Platform/Xbox360)
    target_compile_definitions(lan_peer_loopback_tests PRIVATE INFINITY_LAN_LOOPBACK_TEST=1)
    target_link_libraries(lan_peer_loopback_tests PRIVATE Threads::Threads)
    add_test(NAME lan_peer_loopback COMMAND lan_peer_loopback_tests)
    set_tests_properties(lan_peer_loopback PROPERTIES TIMEOUT 10)
endif()

add_executable(ftp_session_tests Tests/FtpSessionTests.cpp Platform/Xbox360/FtpSession.cpp Platform/Xbox360/FileWorkspace.cpp)
target_include_directories(ftp_session_tests PRIVATE Platform/Xbox360)
add_test(NAME ftp_session COMMAND ftp_session_tests)

# Interoperability test only: Linux loopback, no console or LAN listener.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND Python3_Interpreter_FOUND)
    add_executable(ftp_loopback_probe Tests/FtpLoopbackProbe.cpp Platform/Xbox360/FtpSession.cpp Platform/Xbox360/FileWorkspace.cpp)
    target_include_directories(ftp_loopback_probe PRIVATE Platform/Xbox360)
    add_test(NAME ftp_loopback COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/FtpLoopbackTest.py $<TARGET_FILE:ftp_loopback_probe>)
    set_tests_properties(ftp_loopback PROPERTIES TIMEOUT 30)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND Python3_Interpreter_FOUND)
    add_executable(ftp_server_probe Tests/FtpServerProbe.cpp Platform/Xbox360/FtpSession.cpp Platform/Xbox360/FtpServer.cpp Platform/Xbox360/FileWorkspace.cpp)
    target_include_directories(ftp_server_probe PRIVATE Platform/Xbox360)
    add_test(NAME ftp_server COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/FtpServerTest.py $<TARGET_FILE:ftp_server_probe>)
    set_tests_properties(ftp_server PROPERTIES TIMEOUT 30)
endif()

add_executable(store_package_tests Tests/StorePackageTests.cpp Platform/Xbox360/StorePackage.cpp Platform/Xbox360/FileWorkspace.cpp Platform/Xbox360/StoreCatalog.cpp Platform/Xbox360/MediaUrl.cpp)
target_include_directories(store_package_tests PRIVATE Platform/Xbox360 Source)
target_sources(store_package_tests PRIVATE Platform/Xbox360/DiscInstaller.cpp Platform/Xbox360/StorageInfo.cpp)
target_link_libraries(store_package_tests PRIVATE aurorax_core)
add_test(NAME store_package COMMAND store_package_tests)

# Native ZIP installer regression, using the same pinned decompressor as XSDK.
if(DEFINED INFINITY_MINIZ_DIR AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND Python3_Interpreter_FOUND)
    include(Platform/Xbox360/third_party/MinizReader.cmake)
    infinity_miniz(infinity_zip_reader "${INFINITY_MINIZ_DIR}")
    add_executable(store_zip_probe Tests/StoreZipProbe.cpp Platform/Xbox360/StoreZipInstaller.cpp Platform/Xbox360/StorePackage.cpp Platform/Xbox360/StoreCatalog.cpp Platform/Xbox360/FileWorkspace.cpp Platform/Xbox360/StorageInfo.cpp Platform/Xbox360/MediaUrl.cpp)
    target_include_directories(store_zip_probe PRIVATE Platform/Xbox360 Source)
    target_link_libraries(store_zip_probe PRIVATE infinity_zip_reader aurorax_core)
    add_test(NAME store_zip COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/StoreZipTests.py $<TARGET_FILE:store_zip_probe>)
endif()

add_executable(unity_catalog_tests Tests/UnityCatalogTests.cpp Platform/Xbox360/UnityCatalog.cpp Platform/Xbox360/MediaUrl.cpp)
target_include_directories(unity_catalog_tests PRIVATE Platform/Xbox360 Source)
add_test(NAME unity_catalog COMMAND unity_catalog_tests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(content_backup_tests Tests/ContentBackupTests.cpp Platform/Xbox360/ContentBackup.cpp Platform/Xbox360/ArtImport.cpp Platform/Xbox360/FileWorkspace.cpp Platform/Xbox360/StorageInfo.cpp)
    target_include_directories(content_backup_tests PRIVATE Platform/Xbox360 Source)
    target_link_libraries(content_backup_tests PRIVATE aurorax_core)
    add_test(NAME content_backup COMMAND content_backup_tests)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(content_install_tests Tests/ContentInstallTests.cpp Platform/Xbox360/ContentInstall.cpp Platform/Xbox360/ContentBackup.cpp Platform/Xbox360/FileWorkspace.cpp Platform/Xbox360/StorageInfo.cpp)
    target_include_directories(content_install_tests PRIVATE Platform/Xbox360 Source)
    target_link_libraries(content_install_tests PRIVATE aurorax_core)
    add_test(NAME content_install COMMAND content_install_tests)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(package_import_tests Tests/PackageImportTests.cpp Platform/Xbox360/PackageImport.cpp Platform/Xbox360/FileWorkspace.cpp Platform/Xbox360/StorageInfo.cpp)
    target_include_directories(package_import_tests PRIVATE Platform/Xbox360 Source)
    target_link_libraries(package_import_tests PRIVATE aurorax_core)
    add_test(NAME package_import COMMAND package_import_tests)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND Python3_Interpreter_FOUND)
    find_package(OpenSSL QUIET COMPONENTS Crypto)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND Python3_Interpreter_FOUND AND TARGET OpenSSL::Crypto)
    add_executable(disc_god_probe Tests/DiscGodProbe.cpp Platform/Xbox360/DiscGod.cpp
        Platform/Xbox360/DiscImage.cpp Platform/Xbox360/FileWorkspace.cpp
        Platform/Xbox360/PackageImport.cpp Platform/Xbox360/StorageInfo.cpp)
    target_include_directories(disc_god_probe PRIVATE Platform/Xbox360 Source)
    target_link_libraries(disc_god_probe PRIVATE aurorax_core OpenSSL::Crypto)
    add_test(NAME disc_god COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/DiscGodTests.py $<TARGET_FILE:disc_god_probe>)
endif()

# Portable active profile and chat capability contract (no network calls).
add_executable(social_profile_tests Tests/SocialProfileTests.cpp)
target_include_directories(social_profile_tests PRIVATE Platform/Xbox360)
add_test(NAME social_profile COMMAND social_profile_tests)
add_executable(tv_preferences_tests Tests/TvPreferencesTests.cpp)
target_include_directories(tv_preferences_tests PRIVATE Platform/Xbox360)
add_test(NAME tv_preferences COMMAND tv_preferences_tests)
add_executable(tv_remote_tests Tests/TvRemoteTests.cpp)
target_include_directories(tv_remote_tests PRIVATE Platform/Xbox360)
add_test(NAME tv_remote COMMAND tv_remote_tests)

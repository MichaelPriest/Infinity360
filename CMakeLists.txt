cmake_minimum_required(VERSION 3.16)
project(AuroraX LANGUAGES CXX)

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

# Xbox 360 uses the XDK project in Platform/Xbox360. The native bootstrap is
# intentionally excluded from desktop builds because it depends on XTL/D3D9.

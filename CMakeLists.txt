# Sets the minimum CMake version
cmake_minimum_required(VERSION 3.15)

# Project name and C++ standard (17 is needed for SFML)
project(RECalc VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CRITICAL WORKAROUND: MANUALLY CONFIGURE LIBRARIES FOR STATIC VCPKG BUILD ---
# Use package-config targets provided by vcpkg (preferred).
# Removing hard-coded include/link paths so CMake + vcpkg toolchain can
# supply the correct targets for static or dynamic triplets.
find_package(ImGui-SFML CONFIG REQUIRED)
set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED CONFIG)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

# Create your executable
add_executable(recalc main.cpp)

# 4. Manually link all required static debug libraries and their system dependencies.
# We are manually listing the expected names of static debug libraries.
target_link_libraries(recalc PRIVATE
    ImGui-SFML::ImGui-SFML
    SFML::Graphics
    SFML::Window
    SFML::System
    opengl32
    freetype
    gdi32
    winmm
    ws2_32
)

# Test executable
add_executable(tests tests.cpp)
# Tests don't need SFML/ImGui, just standard C++
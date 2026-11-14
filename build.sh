#!/usr/bin/env sh
# ORI - one-line build for Linux / macOS without make or CMake.
set -e
mkdir -p build
SRC="src/core/*.cpp src/math/*.cpp src/geometry/*.cpp src/raster/*.cpp src/curves/*.cpp src/shading/*.cpp src/editor/*.cpp src/fold/*.cpp src/layering/*.cpp src/models/*.cpp"
UI="demos/*.cpp studio/*.cpp"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o build/ori app/main.cpp demos/*.cpp $SRC
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o build/ori_tests tests/*.cpp $UI $SRC
echo "Built build/ori and build/ori_tests"
# The interactive studio needs SDL2 (only to open the window).
if pkg-config --exists sdl2 2>/dev/null; then
    ${CXX:-g++} -std=c++17 -O2 -Wall -Wextra $(pkg-config --cflags sdl2) -o build/ori_studio studio/sdl/main.cpp $UI $SRC $(pkg-config --libs sdl2)
    echo "Built build/ori_studio"
else
    echo "SDL2 not found - skipped build/ori_studio (install libsdl2-dev or sdl2)"
fi

#!/usr/bin/env sh
# ORI - one-line build for Linux / macOS without make or CMake.
set -e
mkdir -p build
SRC="src/core/*.cpp src/math/*.cpp src/geometry/*.cpp src/raster/*.cpp src/curves/*.cpp src/shading/*.cpp src/editor/*.cpp src/fold/*.cpp src/layering/*.cpp src/models/*.cpp"
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o build/ori app/main.cpp demos/*.cpp $SRC
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o build/ori_tests tests/*.cpp $SRC
echo "Built build/ori and build/ori_tests"

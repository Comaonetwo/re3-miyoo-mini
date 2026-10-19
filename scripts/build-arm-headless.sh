#!/bin/sh
# Standalone static-musl benchmark only. Never use this ABI to link Miyoo SDL.
set -eu
cd "$(dirname "$0")/.."
: "${ZIG:=zig}"
mkdir -p build-arm-static
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-$PWD/build-arm-static/zig-cache}"
"$ZIG" c++ -target arm-linux-musleabihf -mcpu=cortex_a7 -static -std=c++11 -O2 -Wl,--strip-debug \
    src/soft_renderer.cpp src/scene.cpp src/headless.cpp -o build-arm-static/soft_headless
file build-arm-static/soft_headless

#!/bin/sh
# Produces a complete SDL-free screen/button demo for Miyoo Mini / Plus.
set -eu
cd "$(dirname "$0")/.."
: "${ZIG:=zig}"
mkdir -p build-arm-display
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-$PWD/build-arm-display/zig-cache}"
"$ZIG" c++ -target arm-linux-musleabihf -mcpu=cortex_a7 -static -std=c++11 \
    -O2 -Wl,--strip-debug -Wall -Wextra -Wpedantic -pthread \
    src/soft_renderer.cpp src/scene.cpp src/asset_scene.cpp src/texture_renderer.cpp src/collision_world.cpp src/driving.cpp src/checkpoint_run.cpp src/overlay_text.cpp src/fb_blit.cpp src/linux_main.cpp \
    -o build-arm-display/gta3_soft_fb
file build-arm-display/gta3_soft_fb

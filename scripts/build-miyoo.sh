#!/bin/sh
# Run in a Linux container with the actual Miyoo glibc toolchain and built SDL.
set -eu
cd "$(dirname "$0")/.."
: "${MIYOO_TOOLCHAIN:?Set MIYOO_TOOLCHAIN, e.g. /opt/mini}"
: "${MIYOO_SDL_SOURCE:?Set path to pinned steward-fu/sdl2 repository after building it}"
: "${CMAKE:=cmake}"
: "${MIYOO_SDL_INCLUDE:=$MIYOO_SDL_SOURCE/sdl2/include}"
: "${MIYOO_SDL_LIBRARY:=$MIYOO_SDL_SOURCE/sdl2/build/.libs/libSDL2.so}"
[ -f "$MIYOO_SDL_LIBRARY" ] || { echo "Built target SDL missing: $MIYOO_SDL_LIBRARY" >&2;exit 1; }
"$CMAKE" -S . -B build-miyoo -DCMAKE_TOOLCHAIN_FILE=cmake/miyoo.cmake \
    -DCMAKE_BUILD_TYPE=Release -DSOFT_SDL=ON -DSOFT_MIYOO=ON -DBUILD_TESTING=OFF \
    -DSOFT_SDL_INCLUDE="$MIYOO_SDL_INCLUDE" -DSOFT_SDL_LIBRARY="$MIYOO_SDL_LIBRARY"
"$CMAKE" --build build-miyoo -j 2
"$MIYOO_TOOLCHAIN/bin/arm-linux-gnueabihf-readelf" -h -A -d build-miyoo/gta3_soft_demo

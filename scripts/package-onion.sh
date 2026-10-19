#!/bin/sh
# Stage only original demo binary and scripts. SDL/vendor libraries supplied separately.
set -eu
cd "$(dirname "$0")/.."
binary=${1:-build-miyoo/gta3_soft_demo}
[ -f "$binary" ] || { echo "Missing target demo: $binary" >&2;exit 1; }
case $(file -b "$binary") in
  *ELF*ARM*) ;;
  *) echo "Refusing to package a host/non-ARM executable" >&2;exit 1;;
esac
stage=build-onion
[ ! -e "$stage" ] || { echo "$stage already exists; use a fresh staging directory" >&2;exit 1; }
mkdir -p "$stage"
cp -R packaging/Roms "$stage/"
cp "$binary" "$stage/Roms/PORTS/Games/GTA3Soft/gta3_soft_demo"
chmod +x "$stage/Roms/PORTS/Games/GTA3Soft/run.sh" "$stage/Roms/PORTS/Games/GTA3Soft/gta3_soft_demo"
echo "Staged in $stage. Add compatible runtime libraries; see docs/CONSOLE.md."

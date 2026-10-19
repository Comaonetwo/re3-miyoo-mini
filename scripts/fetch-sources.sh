#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p vendor
fetch() {
    url=$1 rev=$2 dst=$3
    if [ -e "$dst/.git" ]; then
        actual=$(git -C "$dst" rev-parse HEAD)
        [ "$actual" = "$rev" ] || { echo "Revision mismatch in $dst; use a fresh directory" >&2; exit 1; }
    else
        if [ -d "$dst" ] && [ -n "$(ls -A "$dst")" ]; then
            echo "Refusing to initialize nonempty directory: $dst" >&2; exit 1
        fi
        git init -q "$dst"
        git -C "$dst" remote add origin "$url"
        git -C "$dst" fetch --depth 1 origin "$rev"
        git -C "$dst" checkout --detach FETCH_HEAD
    fi
}
fetch https://github.com/hottabxp/re3.git 3233ffe1c4b99e8efb4c41c6794b4fce880cf503 vendor/re3
# Other submodules/audio libraries are deliberately not fetched for the probe.
fetch https://github.com/aap/librw.git 5501c4fdc7425ff926be59369a13593bb6c81b54 vendor/re3/vendor/librw
if [ "${1:-}" = "--sdl" ]; then
    fetch https://github.com/steward-fu/sdl2.git 0631abc8e8916db6f9bc7e2afd0c22913d092a29 vendor/miyoo-sdl2
fi

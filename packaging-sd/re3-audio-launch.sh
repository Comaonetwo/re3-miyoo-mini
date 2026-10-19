#!/bin/sh
# Keep the glibc audio process separate from the static musl game.
set -u
gta_audio_here=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 2
unset LD_PRELOAD
gta_audio_runtime=/mnt/SDCARD/.tmp_update/lib/parasyte
gta_audio_libs="$gta_audio_runtime:/mnt/SDCARD/miyoo/lib:/config/lib:/mnt/SDCARD/.tmp_update/lib:/customer/lib:/usr/lib:/lib"
if [ -x "$gta_audio_runtime/ld-linux-armhf.so.3" ] && [ -f "$gta_audio_runtime/libc.so.6" ]; then
 printf 'AUDIO runtime: Onion parasyte loader\n' >&2
 exec "$gta_audio_runtime/ld-linux-armhf.so.3" --library-path "$gta_audio_libs" "$gta_audio_here/re3_audio" "$@"
fi
printf 'AUDIO runtime: firmware loader\n' >&2
export LD_LIBRARY_PATH="$gta_audio_libs"
exec "$gta_audio_here/re3_audio" "$@"

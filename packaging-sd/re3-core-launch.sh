#!/bin/sh
set -u
export LD_LIBRARY_PATH="/mnt/SDCARD/miyoo/lib:/usr/lib:/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$(dirname "$0")" || exit 1
gta_cpu_governor_path=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
gta_prev_governor=""
if [ -r "$gta_cpu_governor_path" ] && [ -w "$gta_cpu_governor_path" ]; then
 gta_prev_governor=$(cat "$gta_cpu_governor_path" 2>/dev/null)
 case "$gta_prev_governor" in performance|ondemand|conservative|powersave|userspace|interactive) ;; *) gta_prev_governor="" ;; esac
 if [ -n "$gta_prev_governor" ]; then printf 'performance\n' > "$gta_cpu_governor_path" 2>/dev/null || true; fi
fi
trap 'if [ -n "$gta_prev_governor" ]; then printf "%s\n" "$gta_prev_governor" > "$gta_cpu_governor_path" 2>/dev/null || true; fi' 0
# Preflight before starting the engine; no game files are installed by this port.
gta_missing=""
for gta_required in game/models/gta3.img game/models/gta3.dir game/models/coll/peds.col game/data/gta3.dat game/data/main.scm game/anim/ped.ifp game/text/american.gxt game/audio/sfx.raw game/audio/sfx.sdt game/audio_pcm/000.pcm game/audio_pcm/195.pcm; do
 if [ ! -s "$gta_required" ]; then gta_missing="$gta_required"; break; fi
done
if [ -n "$gta_missing" ]; then
 printf 'GTA III data missing: %s\nSee _required_files.txt and prepare-game.py.\n' "$gta_missing" > core.log
 printf 'GTA III data missing.\nSee _required_files.txt\nand prepare-game.py.\n' > /tmp/MissingPortFile.tmp
 exit 2
fi
./re3_cpu game --world 0 --console > core.log 2>&1
result=$?
if [ "$result" -eq 42 ]; then
 if [ -d /mnt/SDCARD/.tmp_update ]; then rm -f /tmp/state.json; fi
 result=0
fi
printf '\nLauncher exit code: %s\n' "$result" >> core.log
exit "$result"

# Building the Miyoo port

Requirements: Python3, Git, CMake>=3.20, Zig0.17.0 (the tested compiler).
No GTA III data is needed to compile. Upstream sources are not vendored.

From this repository:

```sh
sh scripts/fetch-sources.sh
python3 scripts/prepare-re3-cpu.py vendor/re3 build/re3-cpu
python3 scripts/build-re3-cpu.py --zig /absolute/path/to/zig --cmake cmake --re3-cpu build/re3-cpu --librw vendor/re3/vendor/librw --pixel-stats off --output build/arm
python3 scripts/package-public.py build/arm/build/re3_cpu build/arm/build/re3_audio dist/gta3-miyoo-v0.63
```

Use a new output path: the packager refuses to replace existing releases.
Pinned upstream revisions are in sources.lock.json; fetching depends on those
third-party repositories remaining available. This is a build recipe, not a
promise of identical binary hashes across every host/toolchain version.

The statically linked game uses musl and LLVM C++ libraries. The separate audio
helper targets glibc2.4 and dynamically loads the console's SDL. Onion's
parasyte loader is used if available, otherwise the firmware loader. No SDL or
Onion firmware libraries are bundled. Helper and kernel must be updated together.

For a native focused menu check (no game resources):

```sh
cmake -S . -B build/menu -DBUILD_TESTING=ON -DSOFT_SANITIZERS=ON
cmake --build build/menu --target about_menu_tests core_menu_preview
ctest --test-dir build/menu -R '^about_menu$' --output-on-failure
```

The project retains historical experimental tests; this recipe does not claim
that all old experiments apply to the release or that every CTest target was
run. Performance work requires controlled identical scenes and whole-frame
measurement; desktop render timings must not be described as Miyoo Mini FPS.

Game-data preparation, separately: scripts/prepare-game.py uses FFmpeg on the
owner's computer and writes private converted files. Never commit its output.

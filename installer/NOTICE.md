# Installer dependencies

Original installer code is MIT-licensed, as described by the port's LICENSE.
The playable engine has separate notices inside port.zip; no game data is bundled.

- CPython 3.13.16, python-build-standalone 20261013: PSF and bundled component
  licenses. See python-mac-licenses/ and python-win-licenses/, including PYTHON.json
  for the exact upstream build metadata and component notices. macOS interpreter
  and library combine the corresponding arm64 and x86_64 builds with lipo.
- pycdlib 1.14.0: LGPL 2.1 or later. Complete editable Python module source is
  supplied in ../vendor/pycdlib; license text is pycdlib-COPYING.
- FFmpeg 7.1.5: LGPL 2.1 or later for this minimal build; GPL/nonfree components
  are not enabled. The exact original source archive is ffmpeg.tar.xz.
  Only WAV/MP3 input and mono S16LE output are needed. The component is a separate
  command-line executable, not linked into the installer UI.
- unshield 1.6.2 snapshot: MIT. Full used source and license in unshield.tar.gz.
- zlib 1.3.1: zlib license; original source and license in zlib.tar.gz. Used by
  the Windows cabinet tool; the macOS tool uses the system zlib.
- Native UIs use system frameworks. Zig links the Windows UI against system DLLs.

Pinned URLs and checksums are in dependencies.json. Build scripts and source
archives are supplied with the installer source release. Replace the separate
FFmpeg/unshield tools or pycdlib source to use modified versions. A modified
macOS bundle can be ad-hoc signed again using `codesign --force --deep --sign -`.
No executable is downloaded at installation time.

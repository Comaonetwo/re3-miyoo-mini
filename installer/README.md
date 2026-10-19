# GTA III Installer

Prepare your own classic two-disc PC edition of GTA III for Miyoo Mini + OnionOS.
The installer contains the port and its tools, but no playable game data.

1. Extract the entire download. On macOS open GTA III Installer.app; on Windows
   open GTA III Installer.exe. Keep the Windows backend folder next to the EXE.
2. Select or drag in GTAIII.iso and GTA3_audio.iso. The original two-disc
   InstallShield edition used in testing is supported; other pressings/layouts
   may be rejected. If the discs are swapped, their roles are detected.
3. Select the root of the Miyoo SD card, with Onion Ports Collection enabled.
4. Click Prepare and install. Wait for completion, then safely eject the card.
5. Launch GTA III (re3) from Ports; refresh the game list if necessary.

No Python or FFmpeg installation is required. The installer works offline.
Allow at least 4 GB of temporary space on the computer and around 2 GB free on
an SD card. Data is prepared on the computer before the card is updated.
Existing saves/settings are copied into the new installation. When updating,
the previous port folder is also kept as a backup on the card; it can be removed
manually after confirming the new installation works. Additional backups consume
space. Never share the prepared game folder or audio files.

macOS: universal application for Apple Silicon and Intel, macOS 13 or newer.
This community build is ad-hoc signed, not notarized with an Apple Developer ID.
If macOS blocks it, use the normal Open Anyway control in System Settings →
Privacy & Security after checking that you downloaded the intended release.
Do not disable Gatekeeper globally.
Windows: portable x64 application intended for Windows 10/11. Components have
been smoke-tested under Wine/CrossOver; testing on a real Windows machine is
still required. No administrator rights are requested.

The installer does not format the card or replace OnionOS. Cancel can be used
while preparing/copying. Keep the card connected during the final installation
step. Unplugging it or losing power can interrupt any installer; the previous
folder and a recovery note are retained during the swap.

Builder instructions (macOS, Xcode command-line tools, Python 3, make, Zig 0.17.0):

```
python3 installer/build_components.py --work /path/without-spaces/installer-build --zig /path/to/zig
python3 installer/build.py --work /path/without-spaces/installer-build --zig /path/to/zig --port-zip /path/to/port-install.zip --output /path/to/new-output
```

Component sources and licensing notices are in installer/third_party and
installer/NOTICE.md. Runtime downloads for builders are checksum-pinned;
installation by end users requires no network connection.

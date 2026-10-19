# GTA III (re3) for Miyoo Mini

An experimental software-rendered port of the reverse-engineered **re3** engine
for the **original Miyoo Mini running OnionOS**. Port version **0.63**.

**A legally purchased copy of the original PC GTA III is required.** Game data,
music, voices and saves are not included. The Definitive Edition, mobile and
console versions are not supported.

This is an unofficial community project, not affiliated with Rockstar Games,
Take-Two, the re3 team or OnionUI.

## Installation

Download the installers and manual package from [Releases](https://github.com/Comaonetwo/re3-miyoo-mini/releases/latest).

1. Enable **Ports Collection** in OnionOS: Apps → Package Manager.
2. Open **GTA III Installer** for macOS or Windows.
3. Select or drag in both supported CD images: the game disc and the audio disc.
4. Select the root of the Miyoo SD card and click **Prepare and install**.
5. When installation finishes, safely eject the card and launch **GTA III (re3)**
   from Ports. Refresh the game list or run **~Import ports** if necessary.

The installer works offline and includes the tools it needs. It prepares your
own game files and audio, then installs the port, shortcut and cover. Existing
saves and settings are preserved. The first release supports the two-disc PC/CD
edition used in testing; other editions and disc layouts are not yet supported.
The macOS app supports Apple Silicon and Intel (macOS 13 or newer). The Windows
app targets Windows 10/11 x64.

For manual installation, extract the port ZIP into the SD-card root, merging
`Roms`, and follow `_required_files.txt` in `Roms/PORTS/Games/GTA3Core14/`.
The command-line preparation tool remains in `tools`. Do not share the generated
`game` folder: it contains your licensed game data.

## Performance and graphics

**Dynamic World is enabled by default.** It adjusts world resolution, draw
distance and rain quality to aim for roughly **18–21 FPS**. Sustained performance
below 18 FPS reduces quality; above 21 FPS it can restore quality. Heavy scenes
can still run more slowly. Draw distance adapts between 80 and 120 metres.

Two graphics presets are available, with individual settings retained through
**CUSTOM**:

- **MEDIUM — recommended.** Cheaper lighting and disabled shadows usually allow
  higher world resolution and longer draw distance at similar smoothness. The
  menu background is 240×180 and the HUD is 320×240.
- **HIGH.** Smoother lighting and shadows closer to the original game, at higher
  cost. Dynamic World may select a lower resolution or shorter draw distance.
  The menu background and HUD are 320×240.

Auto aim is enabled by default. World and HUD resolution can be set separately,
and the FPS overlay can be hidden.

## Controls

Press **MENU** for port settings and **START** for the original pause menu.
**MENU + SELECT** exits to Onion. **HOTKEYS** lists the full controls;
**ABOUT** shows the current port version.

| Action | Control |
|---|---|
| Move / steer | D-pad |
| Sprint / accelerate | A |
| Jump / brake | X |
| Fire with auto aim | B |
| Enter / leave vehicle | Y |
| Rotate camera on foot | L / R |
| Radio / handbrake in a vehicle | L / R |
| Aim / look back on foot | L2 / R2 |
| Vehicle mission / horn | L2 / R2 |

## Status and known issues

Walking, driving, combat, saving/loading and the first missions work in our
tests on an original Miyoo Mini. The full campaign and every gameplay action
have not been verified. Artifacts, bugs and control issues may remain.
The macOS installer and a fresh installation have been tested on a real SD card.
The Windows installer and Miyoo Mini Plus still need testing on real hardware.

Some sound effects can be delayed by roughly **half a second**, as noticed
during playtesting. This is an observation, not a measured upper bound.
Music and radio generally work correctly.

## Development and credits

The Miyoo-specific port was **100% vibecoded** with an AI coding assistant and
playtested on real hardware. The original re3 engine and its libraries were
created by their respective authors. Contributions are welcome.

Thanks to the re3 team, aap/librw and OnionUI. Original port code is MIT-licensed;
re3 has its own upstream notice rather than a conventional software license.
See `LICENSE` and `licenses/`. The menu cover is promotional artwork and is not
covered by the port's MIT license.

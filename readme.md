# Windowed Mode plugin (for GTA3, VC, SA)
[DOWNLOAD](https://github.com/ThirteenAG/III.VC.SA.WindowedMode/releases/latest)
----
This plugin allows to run game in window as well in windowed full screen mode.
Originally created by **maxorator** as part of **Maxo's Vehicle Loader** for Vice City.

----
## Installation
1. [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) is required. If you don't have it, just install [Widescreen Fix](https://thirteenag.github.io/wfp#gta), ASI Loader comes with it.
2. Unpack **III.VC.SA.WindowedMode.asi** into the **scripts** folder, or the [modloader](https://github.com/thelink2012/modloader/releases) folder.
3. Start the game. **III.VC.SA.WindowedMode.cfg** is created next to the plugin with the default settings.

FusionFixHub installs the plugin as an extra of the GTA III, Vice City and San Andreas fixes and edits its settings, also while the game runs.

----
## Supported games
* Classic GTA III v1.0, v1.1 and Steam executables
* Classic GTA Vice City v1.0, v1.1 and Steam executables
* Classic GTA San Andreas v1.0 US

----
## Window modes
* **Fullscreen**: the game's own exclusive fullscreen; the plugin changes nothing.
* **Windowed** (default): a window of the chosen resolution, centered on its monitor, resizable. At the desktop resolution the window's frame lies outside the screen, so it looks like borderless fullscreen.
* **Borderless**: a window without frame covering the monitor, at the desktop resolution.

The game is DPI aware, so the picture isn't scaled or blurred at 125-300 % Windows scaling, and windows have square corners on Windows 11.

## Hotkeys
* **Alt+Enter** or **Ctrl+Enter**: switches between Windowed and Borderless

The window can be resized to any size; the game's picture and aspect ratio follow it.

----
## Settings
Settings are stored next to the plugin in **III.VC.SA.WindowedMode.cfg** and can be changed there, in FusionFixHub, or by in-game overlays. All but switching to or from Fullscreen apply at once.

| Setting | Values | Default |
|---|---|---|
| `MAIN.WindowMode` | 0 Fullscreen, 1 Windowed, 2 Borderless (to or from 0 after a restart) | 1 |
| `MAIN.ConfineCursor` | keeps the cursor inside the window while playing | 1 |
| `MAIN.PauseOnFocusLoss` | opens the pause menu when the window loses the focus | 1 |
| `MAIN.ResumeOnFocus` | closes that pause menu again when the focus returns | 1 |
| `WINDOW.Width`, `WINDOW.Height` | the window's picture size in Windowed mode, 0 = desktop; set by the game's resolution menu and by resizing | 0 |
| `WINDOW.Maximized`, `WINDOW.PositionX`, `WINDOW.PositionY` | saved state: maximized, a point on the monitor the window opens on | 0, -1, -1 |

The presets (Vanilla, Recommended, Enhanced) leave every setting of this plugin as it is: they are all the player's own choices. Settings of older versions (**III.VC.SA.WindowedMode.ini**) aren't read; the ini can be deleted.
## Building
Run `premake5.bat`, then build the generated solution in `build` with Visual Studio 2026 or newer, using **Release / Win32**. Initialize the Git submodules first. The project uses C++ modules; game-specific patterns are in `GTA3.ixx`, `GTAVC.ixx` and `GTASA.ixx`.

For local deployment and debugging, create `.env` beside `premake5.lua`, then rerun `premake5.bat`:

```ini
GTAIII_DIR=Z:\WFP\Games\Grand Theft Auto\GTAIII
GRAND_THEFT_AUTO_VICE_CITY_DIR=Z:\WFP\Games\Grand Theft Auto\Grand Theft Auto Vice City
GTA_SAN_ANDREAS_DIR=Z:\WFP\Games\Grand Theft Auto\GTA San Andreas
```

The `Gta3`, `GtaVC` and `GtaSA` configurations use the corresponding path for debugging and copy the built ASI into that game's `scripts` folder only if the plugin is already installed. Quotes and trailing slashes are supported. Missing keys disable deployment and debugger paths for that game. `Release` remains independent of local game paths. The `.env` file is ignored by Git.

For a versioned build, run `premake5.exe vs2026 --with-version=2.2.0` before building. Without an explicit version, the build uses the current date and time. The product version also includes the Git commit hash.

Build `build/Tools.slnx` first: it builds the settings validator, which checks `source/settings` before every build. Run `data/release.bat` after a Release build to embed the PDB, optionally sign the ASI, and package it with the documentation. Signing uses a base64-encoded PFX in `CODE_SIGNING_PFX` and its password in `CODE_SIGNING_PASSWORD`; when neither credential is configured, ordinary builds remain unsigned. Embedding runs before signing.

GitHub Actions builds pushes and pull requests and uploads the ZIP and ASI. Configure the two signing secrets for signed builds on `master`. To publish a release, run the workflow manually on `master`, select `release: true`, and choose the major/minor/patch increment. Releases use the next semantic version, matching the binary's version resource.

----
## Credits
* **maxorator** - created original [Vehicle Loader for Vice City](http://gtaforums.com/topic/477801-maxos-vehicle-loader/)
* **ThirteenAG** - ported into stand-alone ASI plugin
* **not6** - fixed Alt key problems
* **Miran** - rewritten into version 2.0

----
## Screenshots
![ScreenShot](http://i.imgur.com/ApT8VH4m.jpg) ![ScreenShot](http://i.imgur.com/R3k5fPGm.jpg)

![ScreenShot](http://i.imgur.com/Q3GkXikm.jpg) ![ScreenShot](http://i.imgur.com/wHbbtM0m.jpg)

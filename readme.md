# Windowed Mode plugin (for GTA3, VC, SA)
[DOWNLOAD](https://github.com/ThirteenAG/III.VC.SA.WindowedMode/releases/latest)
----
This plugin allows to run game in window as well in windowed full screen mode.
Originally created by **maxorator** as part of **Maxo's Vehicle Loader** for Vice City.

----
## Installation
1. [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) is required. If you don't have it, just install [Widescreen Fix](https://thirteenag.github.io/wfp#gta), ASI Loader comes with it.
2. Unpack **III.VC.SA.WindowedMode.asi** and **III.VC.SA.WindowedMode.ini** into **scripts** folder, or [modloader](https://github.com/thelink2012/modloader/releases) folder.
3. Start the game.

----
## Supported games
* Classic GTA III v1.0, v1.1 and Steam executables
* Classic GTA Vice City v1.0, v1.1 and Steam executables
* Classic GTA San Andreas v1.0 US

----
## Hotkeys
* **Alt+Enter**: Toggle between borderless-fullscreen and windowed modes
* **Ctrl+Enter**: Toggle between bordered and borderless window styles

----
## Configuration
Settings are stored beside the plugin in **III.VC.SA.WindowedMode.ini**. Window size, position and style are saved automatically. Existing v2 configuration keys remain supported.

## Building
Run `premake5.bat`, then build the generated solution in `build` with Visual Studio 2026 or newer, using **Release / Win32**. Initialize the Git submodules first. The project uses C++ modules; game-specific patterns are in `GTA3.ixx`, `GTAVC.ixx` and `GTASA.ixx`.

For a versioned build, run `premake5.exe vs2026 --with-version=2.2.0` before building. Without an explicit version, the build uses the current date and time. The product version also includes the Git commit hash.

Run `data/release.bat` after a Release build to embed the PDB, optionally sign the ASI, and package it with the INI and documentation. Signing uses a base64-encoded PFX in `CODE_SIGNING_PFX` and its password in `CODE_SIGNING_PASSWORD`; when neither credential is configured, ordinary builds remain unsigned. Embedding runs before signing.

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

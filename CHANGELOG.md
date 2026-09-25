## 2.2

- improved window resizing and Direct3D device reset handling in GTA3, GTA-VC and GTA SA
- fixed resolution changes from the game's display settings
- Alt+Enter now toggles between desktop-sized borderless fullscreen and the previous windowed style
- added Ctrl+Enter to switch between bordered and borderless window styles
- added a 640x480 fallback when the saved windowed resolution matches the desktop resolution
- improved window sizing with DPI scaling and centering on secondary monitors
- deferred post-effect updates until rendering resumes after a device reset
- removed the menu frame limiter from this plugin; it is now provided by Widescreen Fix through the MenuFrameLimit setting

## 2.0
- added error message about unsupported game version
- added error message about missing ASI Loader
- added **auto pause** and **auto resume** features
- removed CoordsManager menu
- simplified settings: Alt+Enter now cycles between all available window styles
- window size and style are now stored in the config file and restored on game start
- proper support of multiple monitors
- always show cursor over the inactive game window
- fixed cursor moving out of the window during gameplay in GTA3 and GTA-VC
- fixed flickering window title text
- added aspect ratio information to window title text
- added snapping to known aspec ratios when resizing the window
- fixed missing window icon in GTA3
- fixed issues with audio and animations in the main menu by implementing a frame limiter for it
- fixed freezes when **Grinch's ImGUI** plugin is used

## 1.17
- fixed incorrect positioning of the window in full mode introduced in 1.16
- always show cursor over inactive game window

## 1.16
- setting of initial window position is now done on game start, not postponed until entering main menu
- last window position is now stored in config file and restored on game start

## 1.15
- fixed mouse cursor not showing up in GTA SA

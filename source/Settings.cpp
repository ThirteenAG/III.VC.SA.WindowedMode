module;
#include "stdafx.h"
#include "Bindings.h"

module WindowedMode;

void WindowedMode::InitConfig()
{
    if (!std::filesystem::exists(config.GetIniPath()))
        SaveConfig();
}

bool WindowedMode::LoadConfig()
{
    // Read old v2 keys as fallbacks. CIniReader is case-sensitive.
    auto read = [this](const char* section, const char* key, const char* oldSection, const char* oldKey, int fallback)
    {
        return config.ReadInteger(section, key, config.ReadInteger(oldSection, oldKey, fallback));
    };
    windowMode = static_cast<WindowMode>(std::clamp(read("MAIN", "WindowMode", "window", "mode", Windowed), int(Min), int(Max)));
    windowedStyle = static_cast<WindowMode>(std::clamp(read("MAIN", "WindowedStyle", "window", "windowedStyle",
        windowMode == Fullscreen ? Windowed : windowMode), int(Windowed), int(WindowedBorderless)));
    if (windowMode != Fullscreen) windowedStyle = windowMode;

    windowPosWindowed.x = read("WINDOW", "PositionX", "window", "positionX", -1);
    windowPosWindowed.y = read("WINDOW", "PositionY", "window", "positionY", -1);
    windowSizeWindowed.x = max(read("WINDOW", "ResolutionX", "window", "resolutionX", Resolution_Default.x), Resolution_Min.x);
    windowSizeWindowed.y = max(read("WINDOW", "ResolutionY", "window", "resolutionY", Resolution_Default.y), Resolution_Min.y);
    windowPos = windowPosWindowed;
    windowSize = windowSizeClient = windowSizeWindowed;
    autoPause = read("MAIN", "AutoPause", "game", "autoPause", 1) != 0;
    autoResume = read("MAIN", "AutoResume", "game", "autoResume", 1) != 0;
    return read("WINDOW", "Maximized", "window", "maximized", 0) != 0;
}

void WindowedMode::SaveConfig()
{
    // Batch persistent window state into one write while preserving comments
    // and unrelated settings, including the old keys during migration.
    mINI::INIFile file(config.GetIniPath());
    mINI::INIStructure ini;
    file.read(ini);
    auto set = [&ini](const char* section, const char* key, int value)
    {
        ini[section][key] = std::to_string(value);
    };
    set("MAIN", "WindowMode", windowMode);
    set("MAIN", "WindowedStyle", windowedStyle);
    set("MAIN", "AutoPause", autoPause);
    set("MAIN", "AutoResume", autoResume);
    set("WINDOW", "Maximized", window && IsZoomed(window));
    set("WINDOW", "PositionX", windowPosWindowed.x);
    set("WINDOW", "PositionY", windowPosWindowed.y);
    set("WINDOW", "ResolutionX", windowSizeWindowed.x);
    set("WINDOW", "ResolutionY", windowSizeWindowed.y);
    file.write(ini, true);
}

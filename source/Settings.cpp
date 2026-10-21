module;
#include "stdafx.h"
#include "Bindings.h"
#include "Settings.h"

module WindowedMode;

void WindowedMode::LoadConfig()
{
    auto& settings = Settings();
    windowMode = static_cast<WindowMode>(std::clamp(settings.Int("MAIN.WindowMode"), int(Windowed), int(Borderless)));
    windowSizeWindowed = { settings.Int("WINDOW.Width"), settings.Int("WINDOW.Height") };
    if (windowSizeWindowed.x > 0 && windowSizeWindowed.y > 0)
    {
        windowSizeWindowed.x = max(windowSizeWindowed.x, Resolution_Min.x);
        windowSizeWindowed.y = max(windowSizeWindowed.y, Resolution_Min.y);
    }
    else
        windowSizeWindowed = {};
    windowMonitorPoint = { settings.Int("WINDOW.PositionX"), settings.Int("WINDOW.PositionY") };
    windowMaximized = settings.Bool("WINDOW.Maximized");
}

void WindowedMode::SaveConfig()
{
    // Only changed values are stored: every Set writes the .cfg
    auto& settings = Settings();
    auto store = [&settings](const char* id, int value)
    {
        if (settings.Int(id) != value)
            settings.Set(id, std::to_string(value));
    };
    if (window && !IsIconic(window))
    {
        windowMaximized = IsZoomed(window) != FALSE;
        RECT rect{};
        if (GetWindowRect(window, &rect))
            windowMonitorPoint = { (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
    }
    store("MAIN.WindowMode", windowMode);
    store("WINDOW.Width", windowSizeWindowed.x);
    store("WINDOW.Height", windowSizeWindowed.y);
    store("WINDOW.Maximized", windowMaximized);
    store("WINDOW.PositionX", windowMonitorPoint.x);
    store("WINDOW.PositionY", windowMonitorPoint.y);
}

// Runs on the window's thread for changes from editors, presets and the in-game overlay
void WindowedMode::ApplySettings()
{
    auto& settings = Settings();
    auto mode = settings.Int("MAIN.WindowMode");
    // Fullscreen is the game's own mode: switching to it applies after a restart
    if (mode != Fullscreen && mode != windowMode)
        WindowSetMode(static_cast<WindowMode>(std::clamp(mode, int(Windowed), int(Borderless))));

    POINT size = { settings.Int("WINDOW.Width"), settings.Int("WINDOW.Height") };
    if (size.x <= 0 || size.y <= 0)
        size = {};
    if (size.x != windowSizeWindowed.x || size.y != windowSizeWindowed.y)
    {
        windowSizeWindowed = size;
        if (windowMode == Windowed)
        {
            if (IsZoomed(window))
            {
                windowUpdating = true;
                ShowWindow(window, SW_RESTORE);
                windowUpdating = false;
            }
            WindowCalculateGeometry(true);
        }
    }
    MouseUpdate(true);
}

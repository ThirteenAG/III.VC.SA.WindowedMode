module;
#include "Bindings.h"
#include "Settings.h"

export module Game;

import WindowedMode;
import GTA3;
import GTAVC;
import GTASA;

namespace
{
    std::unique_ptr<WindowedMode> controller;

    void __cdecl ChangeResolution(DWORD mode)
    {
        inst->ChangeResolution(mode);
    }

    // Everything else is found and patched when the game creates its window, the same moment for every loader and
    // every folder the plugin is in. III and VC share the same Win32 class name.
    void Start()
    {
        GameBindings game;
        if (!hook::pattern("83 3D ? ? ? ? 08 75 ? E8 ? ? ? ? 85 C0 89 44 24 08").empty())
            game = ResolveGTASA();
        else if (!hook::pattern("8B 35 ? ? ? ? 6A 00 55 6A 00").empty())
            game = ResolveGTAVC();
        else
            game = ResolveGTA3();
        ResolveD3DCalls(game);

        controller = std::make_unique<WindowedMode>(game);
        inst = controller.get();
        // Changes from editors and overlays come from other threads: apply them on the window's thread
        for (auto id : { "MAIN.WindowMode", "MAIN.ConfineCursor", "WINDOW.Width", "WINDOW.Height" })
        {
            Settings().OnChange(id, []()
            {
                if (inst && inst->window)
                    PostMessageA(inst->window, WindowedMode::WM_APPLY_SETTINGS, 0, 0);
            });
        }
        if (game.title == GameTitle::GTASA)
        {
            injector::MakeCALL(game.menuResolutionCall, WindowedMode::ChangeResolutionSA, true);
            injector::MakeCALL(game.changeVideoModeCall, WindowedMode::ChangeVideoModeSA, true);
            injector::WriteMemory<uint8_t>(game.deviceDialog, 0xEB, true);
            if (game.mouseRecenter) injector::MakeNOP(game.mouseRecenter, 5, true);
            injector::MakeJMP(game.grabScreen, WindowedMode::GrabScreenSA, true);
        }
        else
        {
            injector::MakeCALL(game.menuResolutionCall, ChangeResolution, true);
            if (game.title == GameTitle::GTA3)
            {
                injector::WriteMemory<uint8_t>(game.resolutionColor, 0xEB, true);
                injector::MakeNOP(game.resolutionEnable, 6, true);
            }
            else
            {
                injector::WriteMemory<uint16_t>(game.resolutionColor, 0xE990, true);
                injector::MakeNOP(game.resolutionEnable, 2, true);
            }
        }
    }

    HWND __stdcall CreateGameWindow(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style, int x, int y, int width,
        int height, HWND parent, HMENU menu, HINSTANCE module, LPVOID param)
    {
        if (!controller)
            Start();
        return WindowedMode::InitWindow(exStyle, className, title, style, x, y, width, height, parent, menu, module, param);
    }
}

// Only the window creation is patched while the plugin loads (inside DllMain, where a crash ends the game without a
// crash report). A plugin loaded after the window exists (Miles loads *.asi files from the game folder of III and VC
// when the audio starts) changes nothing.
export void InitializeGame()
{
    // Fullscreen is the game's own exclusive mode: nothing to change
    if (Settings().Int("MAIN.WindowMode") == WindowedMode::Fullscreen)
        return;
    // The window is created through the imported CreateWindowExA, or through the function of a plugin
    // that replaced this call before; the window keeps going through it
    auto windowCall = reinterpret_cast<uintptr_t>(hook::get_pattern(windowCreationPattern, 9));
    if (*reinterpret_cast<uint8_t*>(windowCall) == 0xE8)
        WindowedMode::createWindow = reinterpret_cast<decltype(&CreateWindowExA)>(CallTarget(windowCall));
    else
        WindowedMode::createWindow = *ReadOperand<decltype(&CreateWindowExA)*>(windowCall, 2);
    injector::MakeNOP(windowCall, 6, true);
    injector::MakeCALL(windowCall, CreateGameWindow, true);
}

module;
#include "Bindings.h"

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
}

export void InitializeGame(bool sanAndreas)
{
    if (controller) return;
    // Resolve the complete profile first, so an unsupported executable receives
    // no partial menu patches. III and VC share the same Win32 class name.
    GameBindings game;
    if (sanAndreas)
        game = ResolveGTASA();
    else if (!hook::pattern("8B 35 ? ? ? ? 6A 00 55 6A 00").empty())
        game = ResolveGTAVC();
    else
        game = ResolveGTA3();

    controller = std::make_unique<WindowedMode>(game);
    inst = controller.get();
    if (game.title == GameTitle::GTASA)
    {
        injector::MakeCALL(game.menuResolutionCall, WindowedMode::ChangeResolutionSA, true);
        injector::MakeCALL(game.changeVideoModeCall, WindowedMode::ChangeVideoModeSA, true);
        injector::WriteMemory<uint8_t>(game.deviceDialog, 0xEB, true);
        injector::MakeNOP(game.mouseRecenter, 5, true);
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

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

export void InitializeGame()
{
    if (controller) return;
    // Resolve the complete profile first, so an unsupported executable receives
    // no partial menu patches. III and VC share the same Win32 class name.
    GameBindings game;
    if (!hook::pattern("83 3D ? ? ? ? 08 75 ? E8 ? ? ? ? 85 C0 89 44 24 08").empty())
        game = ResolveGTASA();
    else if (!hook::pattern("8B 35 ? ? ? ? 6A 00 55 6A 00").empty())
        game = ResolveGTAVC();
    else
        game = ResolveGTA3();

    ResolveD3DCalls(game);

    auto windowCall = FindPattern("68 00 00 CF 00 ? ? 6A 00 FF 15 ? ? ? ?") + 9;
    auto windowImport = ReadOperand<decltype(&CreateWindowExA)*>(windowCall, 2);
    WindowedMode::createWindow = *windowImport;

    controller = std::make_unique<WindowedMode>(game);
    inst = controller.get();
    // Replace only the executable's six-byte indirect call, preserving its
    // existing imported target (including any other plugin's replacement).
    injector::MakeNOP(windowCall, 6, true);
    injector::MakeCALL(windowCall, WindowedMode::InitWindow, true);
    if (game.title == GameTitle::GTASA)
    {
        injector::MakeCALL(game.menuResolutionCall, WindowedMode::ChangeResolutionSA, true);
        injector::MakeCALL(game.changeVideoModeCall, WindowedMode::ChangeVideoModeSA, true);
        injector::WriteMemory<uint8_t>(game.deviceDialog, 0xEB, true);
        if (game.mouseRecenter) injector::MakeNOP(game.mouseRecenter, 5, true);
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

module;
#include "Bindings.h"

export module GTAVC;

export GameBindings ResolveGTAVC()
{
    GameBindings game;
    game.title = GameTitle::GTAVC;

    game.state = *hook::get_pattern<GameState*>("8B 15 ? ? ? ? 89 D0 83 F8 09 0F 87 ? ? ? ? FF 24 85 ? ? ? ?", 2);
    game.globals = *hook::get_pattern<RsGlobalType*>("8B 35 ? ? ? ? 6A 00 55 6A 00", 2);
    game.modes = *hook::get_pattern<DisplayMode**>("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 0C 81 8B D1", 2);
    game.getNumVideoModes = VideoModeQuery(5);
    game.getCurrentVideoMode = VideoModeQuery(10);
    game.menuActive = *hook::get_pattern<bool*>("80 3D ? ? ? ? 00 75 ? 80 3D ? ? ? ? 00 74 ? 30 C0 EB ? B0 01 89 E9", 2);
    // CMenuManager::m_bStartUpFrontEndRequested (+0x12) next to m_bMenuActive (+0x38). The code that sets it is
    // changed by focus plugins.
    game.menuOpen = game.menuActive - (0x38 - 0x12);
    game.menuClose = game.menuOpen - 1;
    game.camera = *hook::get_pattern<RwCamera**>("FF 35 ? ? ? ? E8 ? ? ? ? 59 59 0F B6 4B 76 69 C9 CC 01 00 00", 2);
    // Found past the bytes a hook at its start replaces: other plugins hook the function
    game.blurOpen = hook::get_pattern<void(RwCamera*)>("83 EC 28 8D 7C 24 10 8B 5C 24 38 A5 A5 A5 A5 83 3D ? ? ? ? 00", -8);
    game.menuResolutionCall = reinterpret_cast<uintptr_t>(hook::get_pattern("8B 45 60 3B 45 5C 0F 84 ? ? ? ? 89 45 5C FF 75 5C E8", 18));
    game.resolutionColor = reinterpret_cast<uintptr_t>(hook::get_pattern("80 7D 6C 00 59 8D 9C 24 94 00 00 00 59 0F 85 ? ? ? ? 8B 85 08 01 00 00 3D FF 00 00 00 7C ?", 13));
    game.resolutionEnable = reinterpret_cast<uintptr_t>(hook::get_pattern("80 7D 6C 00 74 ? E8 ? ? ? ? 80 BC 24 84 00 00 00 00 89 C6 7E ? FF 45 60 8B 45 60 83 3C 86 00 0F 84 ? ? ? ?", 4));
    return game;
}

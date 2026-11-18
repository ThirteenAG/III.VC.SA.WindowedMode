module;
#include "Bindings.h"

export module GTA3;

export GameBindings ResolveGTA3()
{
    GameBindings game;
    game.title = GameTitle::GTA3;

    // The game state's first value in WinMain: intro skipping plugins change the value
    game.state = *hook::get_pattern<GameState*>("C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? C7 44 24 04 94 00 00 00", 2);
    game.globals = *hook::get_pattern<RsGlobalType*>("8B 0D ? ? ? ? 6A ? 6A ? 52", 2);
    // The video mode list: the Steam build reads it differently
    auto modes = hook::pattern("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 0C 81 8B D1");
    game.modes = !modes.empty() ? *modes.get_first<DisplayMode**>(2) :
        *hook::get_pattern<DisplayMode**>("A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 15 ? ? ? ? 83 C4 F8 8D 54 92 05", 1);
    game.getNumVideoModes = VideoModeQuery(5);
    game.getCurrentVideoMode = VideoModeQuery(10);
    game.menuActive = *hook::get_pattern<bool*>("80 3D ? ? ? ? 00 74 ? 30 C0 EB ? B0 01 8B 4C 24 04", 2);
    // Found past the bytes a hook at their start replaces: other plugins hook these functions
    game.requestMenuOpen = hook::get_pattern<void()>("C6 05 ? ? ? ? 01 83 C4 08 C3 00 00 00 00 00 00", -7);
    game.requestMenuClose = hook::get_pattern<void()>("B9 ? ? ? ? C6 05 ? ? ? ? 01 6A 01 E8", -7);
    game.camera = *hook::get_pattern<RwCamera**>("A1 ? ? ? ? 50 DD D8 E8 ? ? ? ? 59 59 83 C4 50", 1);
    game.blurOpen = hook::get_pattern<void(RwCamera*)>("83 EC 28 8D 7C 24 10 8B 5C 24 38 A5 A5 A5 A5 83 3D ? ? ? ? 00", -8);
    game.menuResolutionCall = reinterpret_cast<uintptr_t>(hook::get_pattern("8B 45 04 3B 45 00 0F 84 ? ? ? ? 89 45 00 8B 45 00 50 E8", 19));
    game.resolutionColor = reinterpret_cast<uintptr_t>(hook::get_pattern("75 ? 80 BD 16 01 00 00 00 75 ? 80 7C 24 08 01 75 ? 89 E9 68 FF 00 00 00 E8 ? ? ? ?"));
    game.resolutionEnable = reinterpret_cast<uintptr_t>(hook::get_pattern("80 BD 16 01 00 00 00 0F 84 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? 89 C3 6A 00 68 97 00 00 00", 7));
    return game;
}

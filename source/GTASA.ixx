module;
#include "Bindings.h"

export module GTASA;

export GameBindings ResolveGTASA()
{
    GameBindings game;
    game.title = GameTitle::GTASA;

    game.state = *hook::get_pattern<GameState*>("83 3D ? ? ? ? 08 75 ? E8 ? ? ? ? 85 C0 89 44 24 08", 2);
    game.globals = *hook::get_pattern<RsGlobalType*>("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 80 02 00 00", 2);
    game.modes = *hook::get_pattern<DisplayMode**>("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 04 81 8B D0", 2);
    game.getNumVideoModes = VideoModeQuery(5);
    game.getCurrentVideoMode = VideoModeQuery(10);
    game.menuActive = *hook::get_pattern<bool*>("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 85 C0 0F 85 ? ? ? ? A0 ? ? ? ? 84 C0 75 ?", 1);
    // CGame::InitAfterLostFocus, found past the bytes a hook at its start replaces: focus plugins hook it
    game.menuOpen = *hook::get_pattern<bool*>("84 C0 75 10 A0 ? ? ? ? 84 C0 75 07 C6 05 ? ? ? ? 01 C3", 15);
    game.menuClose = game.menuOpen - 1;
    game.menuResolutionCall = reinterpret_cast<uintptr_t>(hook::get_pattern("50 89 86 D4 00 00 00 E8 ? ? ? ? 83 C4 04 8B CE E8 ? ? ? ? 8B CE 88 9E B8 00 00 00", 7));
    game.selectVideoMode = reinterpret_cast<decltype(game.selectVideoMode)>(CallTarget(game.menuResolutionCall));
    // The video mode change, found past the bytes a hook at its start replaces: menu plugins hook it
    auto mode = reinterpret_cast<uintptr_t>(hook::get_pattern("E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? A1 ? ? ? ? 8B 48 60 8B 51 0C"));
    game.changeVideoModeCall = mode;
    game.changeVideoMode = reinterpret_cast<decltype(game.changeVideoMode)>(CallTarget(mode));
    game.setupBackBuffer = reinterpret_cast<decltype(game.setupBackBuffer)>(CallTarget(mode + 8));
    game.camera = ReadOperand<RwCamera**>(mode, 14);
    game.deviceDialog = reinterpret_cast<uintptr_t>(hook::get_pattern("BF 01 00 00 00 3B C7 7E ? A1 ? ? ? ? 85 C0 89 3D ? ? ? ? 75 ? 6A 00 68 ? ? ? ?", 7));
    // The screen grab of photos and screenshots (replaced as a whole, found past its first bytes), and its image allocation
    game.grabScreen = reinterpret_cast<uintptr_t>(hook::get_pattern("E8 ? ? ? ? 8D 4C 24 14 51 8B F0 8B 06 6A 00 56 FF 50 20", -5));
    auto imageCall = reinterpret_cast<uintptr_t>(hook::get_pattern("6A 20 52 50 E8 ? ? ? ? 8B E8 83 C4 0C 85 ED 74 ? 57 55 E8", 4));
    game.imageCreate = reinterpret_cast<decltype(game.imageCreate)>(CallTarget(imageCall));
    game.imageAllocatePixels = reinterpret_cast<decltype(game.imageAllocatePixels)>(CallTarget(imageCall + 16));
    // Anchored on the mouse coordinates, not the following rendering calls: other plugins replace those calls.
    // The centering call after them is removed by some plugins already.
    auto recenter = hook::get_pattern<uint8_t>("DB 05 ? ? ? ? 8D 44 24 00 50 D8 0D ? ? ? ? D9 5C 24 04 DB 05 ? ? ? ? D8 0D ? ? ? ? D9 5C 24 08", 37);
    if (recenter[0] == 0xE8)
        game.mouseRecenter = reinterpret_cast<uintptr_t>(recenter);
    return game;
}

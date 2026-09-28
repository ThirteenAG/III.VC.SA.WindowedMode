module;
#include "Bindings.h"

export module GTASA;

export GameBindings ResolveGTASA()
{
    GameBindings game;
    game.title = GameTitle::GTASA;
    game.state = ReadOperand<GameState*>(FindPattern("83 3D ? ? ? ? 08 75 ? E8 ? ? ? ? 85 C0 89 44 24 08"), 2);
    game.globals = ReadOperand<RsGlobalType*>(FindPattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 80 02 00 00"), 2);
    game.modes = ReadOperand<DisplayMode**>(FindPattern("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 04 81 8B D0"), 2);
    game.getNumVideoModes = reinterpret_cast<decltype(game.getNumVideoModes)>(FindPattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 05 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 05 00 00 00"));
    game.getCurrentVideoMode = reinterpret_cast<decltype(game.getCurrentVideoMode)>(FindPattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 0A 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 0A 00 00 00"));
    game.menuActive = ReadOperand<bool*>(FindPattern("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 85 C0 0F 85 ? ? ? ? A0 ? ? ? ? 84 C0 75 ?"), 1);
    game.menuOpen = ReadOperand<bool*>(FindPattern("A0 ? ? ? ? 84 C0 75 10 A0 ? ? ? ? 84 C0 75 07 C6 05 ? ? ? ? 01 C3"), 20);
    game.menuClose = game.menuOpen - 1;
    game.menuResolutionCall = FindPattern("50 89 86 D4 00 00 00 E8 ? ? ? ? 83 C4 04 8B CE E8 ? ? ? ? 8B CE 88 9E B8 00 00 00") + 7;
    game.selectVideoMode = reinterpret_cast<decltype(game.selectVideoMode)>(CallTarget(game.menuResolutionCall));
    auto mode = FindPattern("8B 44 24 04 50 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? A1 ? ? ? ? 8B 48 60 8B 51 0C");
    game.changeVideoModeCall = mode + 5;
    game.changeVideoMode = reinterpret_cast<decltype(game.changeVideoMode)>(CallTarget(game.changeVideoModeCall));
    game.setupBackBuffer = reinterpret_cast<decltype(game.setupBackBuffer)>(CallTarget(mode + 13));
    game.camera = ReadOperand<RwCamera**>(mode, 19);
    game.deviceDialog = FindPattern("BF 01 00 00 00 3B C7 7E ? A1 ? ? ? ? 85 C0 89 3D ? ? ? ? 75 ? 6A 00 68 ? ? ? ?") + 7;
    game.mouseRecenter = FindPattern("D9 5C 24 08 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8") + 4;
    return game;
}

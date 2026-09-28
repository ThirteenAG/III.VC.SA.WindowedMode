module;
#include "Bindings.h"

export module GTA3;

export GameBindings ResolveGTA3()
{
    GameBindings game;
    game.title = GameTitle::GTA3;
    game.state = ReadOperand<GameState*>(FindPattern("C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? C7 44 24 04 94 00 00 00"), 2);
    game.globals = ReadOperand<RsGlobalType*>(FindPattern("8B 0D ? ? ? ? 6A ? 6A ? 52"), 2);
    auto modes = hook::pattern("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 0C 81 8B D1");
    game.modes = modes.empty() ? ReadOperand<DisplayMode**>(FindPattern("A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 15 ? ? ? ? 83 C4 F8 8D 54 92 05"), 1) :
        ReadOperand<DisplayMode**>(FindPattern("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 0C 81 8B D1"), 2);
    game.getNumVideoModes = reinterpret_cast<decltype(game.getNumVideoModes)>(FindPattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 05 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 05 00 00 00"));
    game.getCurrentVideoMode = reinterpret_cast<decltype(game.getCurrentVideoMode)>(FindPattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 0A 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 0A 00 00 00"));
    game.menuActive = ReadOperand<bool*>(FindPattern("80 3D ? ? ? ? 00 74 ? 30 C0 EB ? B0 01 8B 4C 24 04"), 2);
    game.requestMenuOpen = reinterpret_cast<decltype(game.requestMenuOpen)>(FindPattern("83 EC 08 89 4C 24 04 C6 05 ? ? ? ? 01 83 C4 08 C3"));
    game.requestMenuClose = reinterpret_cast<decltype(game.requestMenuClose)>(FindPattern("83 EC 08 89 4C 24 04 B9 ? ? ? ? C6 05 ? ? ? ? 01 6A 01 E8"));
    game.camera = ReadOperand<RwCamera**>(FindPattern("A1 ? ? ? ? 50 DD D8 E8 ? ? ? ? 59 59 83 C4 50"), 1);
    game.blurOpen = reinterpret_cast<decltype(game.blurOpen)>(FindPattern("53 56 57 BE ? ? ? ? 83 EC 28 8D 7C 24 10 8B 5C 24 38 A5 A5 A5 A5 83 3D ? ? ? ? 00"));
    game.menuResolutionCall = FindPattern("8B 45 04 3B 45 00 0F 84 ? ? ? ? 89 45 00 8B 45 00 50 E8") + 19;
    game.resolutionColor = FindPattern("75 ? 80 BD 16 01 00 00 00 75 ? 80 7C 24 08 01 75 ? 89 E9 68 FF 00 00 00 E8 ? ? ? ?");
    game.resolutionEnable = FindPattern("80 BD 16 01 00 00 00 0F 84 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? 89 C3 6A 00 68 97 00 00 00") + 7;
    return game;
}

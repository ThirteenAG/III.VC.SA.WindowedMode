module;
#include "Bindings.h"

export module GTAVC;

export GameBindings ResolveGTAVC()
{
    GameBindings game;
    game.title = GameTitle::GTAVC;
    game.state = ReadOperand<GameState*>(FindPattern("8B 15 ? ? ? ? 89 D0 83 F8 09 0F 87 ? ? ? ? FF 24 85 ? ? ? ?"), 2);
    game.globals = ReadOperand<RsGlobalType*>(FindPattern("8B 35 ? ? ? ? 6A 00 55 6A 00"), 2);
    game.modes = ReadOperand<DisplayMode**>(FindPattern("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 0C 81 8B D1"), 2);
    game.getNumVideoModes = reinterpret_cast<decltype(game.getNumVideoModes)>(FindPattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 05 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 05 00 00 00"));
    game.getCurrentVideoMode = reinterpret_cast<decltype(game.getCurrentVideoMode)>(FindPattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 0A 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 0A 00 00 00"));
    game.menuActive = ReadOperand<bool*>(FindPattern("80 3D ? ? ? ? 00 75 ? 80 3D ? ? ? ? 00 74 ? 30 C0 EB ? B0 01 89 E9"), 2);
    game.menuOpen = ReadOperand<bool*>(FindPattern("80 3D ? ? ? ? 00 75 10 80 3D ? ? ? ? 00 75 07 C6 05 ? ? ? ? 01 C3"), 20);
    game.menuClose = game.menuOpen - 1;
    game.camera = ReadOperand<RwCamera**>(FindPattern("FF 35 ? ? ? ? E8 ? ? ? ? 59 59 0F B6 4B 76 69 C9 CC 01 00 00"), 2);
    game.blurOpen = reinterpret_cast<decltype(game.blurOpen)>(FindPattern("53 56 57 BE ? ? ? ? 83 EC 28 8D 7C 24 10 8B 5C 24 38 A5 A5 A5 A5 83 3D ? ? ? ? 00"));
    game.menuResolutionCall = FindPattern("8B 45 60 3B 45 5C 0F 84 ? ? ? ? 89 45 5C FF 75 5C E8") + 18;
    game.resolutionColor = FindPattern("80 7D 6C 00 59 8D 9C 24 94 00 00 00 59 0F 85 ? ? ? ? 8B 85 08 01 00 00 3D FF 00 00 00 7C ?") + 13;
    game.resolutionEnable = FindPattern("80 7D 6C 00 74 ? E8 ? ? ? ? 80 BC 24 84 00 00 00 00 89 C6 7E ? FF 45 60 8B 45 60 83 3C 86 00 0F 84 ? ? ? ?") + 4;
    return game;
}

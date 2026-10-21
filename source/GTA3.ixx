module;
#include "Bindings.h"

export module GTA3;

export GameBindings ResolveGTA3()
{
    GameBindings game;
    game.title = GameTitle::GTA3;

    auto state = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? C7 44 24 04 94 00 00 00");
    auto globals = hook::pattern("8B 0D ? ? ? ? 6A ? 6A ? 52");
    auto modes = hook::pattern("8B 0D ? ? ? ? A3 ? ? ? ? 8D 04 80 8D 0C 81 8B D1");
    auto modesSteam = hook::pattern("A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 15 ? ? ? ? 83 C4 F8 8D 54 92 05");
    auto getNumVideoModes = find_pattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 05 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 05 00 00 00");
    auto getCurrentVideoMode = find_pattern("51 8B 0D ? ? ? ? 6A 00 8D 44 24 04 6A 00 50 83 C1 10 6A 0A 51 E8", "83 EC 1C A1 ? ? ? ? C7 04 24 0A 00 00 00");
    auto menuActive = hook::pattern("80 3D ? ? ? ? 00 74 ? 30 C0 EB ? B0 01 8B 4C 24 04");
    auto requestMenuOpen = hook::pattern("83 EC 08 89 4C 24 04 C6 05 ? ? ? ? 01 83 C4 08 C3");
    auto requestMenuClose = hook::pattern("83 EC 08 89 4C 24 04 B9 ? ? ? ? C6 05 ? ? ? ? 01 6A 01 E8");
    auto camera = hook::pattern("A1 ? ? ? ? 50 DD D8 E8 ? ? ? ? 59 59 83 C4 50");
    auto blurOpen = hook::pattern("53 56 57 BE ? ? ? ? 83 EC 28 8D 7C 24 10 8B 5C 24 38 A5 A5 A5 A5 83 3D ? ? ? ? 00");
    auto menuResolution = hook::pattern("8B 45 04 3B 45 00 0F 84 ? ? ? ? 89 45 00 8B 45 00 50 E8");
    auto resolutionColor = hook::pattern("75 ? 80 BD 16 01 00 00 00 75 ? 80 7C 24 08 01 75 ? 89 E9 68 FF 00 00 00 E8 ? ? ? ?");
    auto resolutionEnable = hook::pattern("80 BD 16 01 00 00 00 0F 84 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? 89 C3 6A 00 68 97 00 00 00");

    // Every binding is resolved before any game code is patched; an ambiguous match counts as missing
    if (modes.size() != 1 && modesSteam.size() != 1)
        throw std::runtime_error("The video mode list wasn't found.");
    for (auto pattern : { &state, &globals, &getNumVideoModes, &getCurrentVideoMode, &menuActive, &requestMenuOpen,
        &requestMenuClose, &camera, &blurOpen, &menuResolution, &resolutionColor, &resolutionEnable })
    {
        if (pattern->size() != 1)
            throw std::runtime_error("Missing or ambiguous executable pattern.");
    }

    game.state = ReadOperand<GameState*>(state.get_first(), 2);
    game.globals = ReadOperand<RsGlobalType*>(globals.get_first(), 2);
    game.modes = modes.size() == 1 ? ReadOperand<DisplayMode**>(modes.get_first(), 2) :
        ReadOperand<DisplayMode**>(modesSteam.get_first(), 1);
    game.getNumVideoModes = getNumVideoModes.get_first<DWORD()>();
    game.getCurrentVideoMode = getCurrentVideoMode.get_first<DWORD()>();
    game.menuActive = ReadOperand<bool*>(menuActive.get_first(), 2);
    game.requestMenuOpen = requestMenuOpen.get_first<void()>();
    game.requestMenuClose = requestMenuClose.get_first<void()>();
    game.camera = ReadOperand<RwCamera**>(camera.get_first(), 1);
    game.blurOpen = blurOpen.get_first<void(RwCamera*)>();
    game.menuResolutionCall = reinterpret_cast<uintptr_t>(menuResolution.get_first(19));
    game.resolutionColor = reinterpret_cast<uintptr_t>(resolutionColor.get_first());
    game.resolutionEnable = reinterpret_cast<uintptr_t>(resolutionEnable.get_first(7));
    return game;
}

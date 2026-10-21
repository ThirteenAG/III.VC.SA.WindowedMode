#pragma once
#include "stdafx.h"

enum class GameTitle { GTA3, GTAVC, GTASA };

enum class ResetResult { Eax, Ebx, Esi, Stack16 };
struct ResetCompletion
{
    uintptr_t address;
    ResetResult result;
};

struct GameBindings
{
    GameTitle title{};
    GameState* state = nullptr;
    RsGlobalType* globals = nullptr;
    DisplayMode** modes = nullptr;
    DWORD (*getNumVideoModes)() = nullptr;
    DWORD (*getCurrentVideoMode)() = nullptr;
    bool* menuActive = nullptr;
    bool* menuOpen = nullptr;
    bool* menuClose = nullptr;
    void (*requestMenuOpen)() = nullptr;
    void (*requestMenuClose)() = nullptr;
    RwCamera** camera = nullptr;
    void (*blurOpen)(RwCamera*) = nullptr;
    void (*setupBackBuffer)() = nullptr;
    void (*selectVideoMode)(DWORD) = nullptr;
    int (*changeVideoMode)(DWORD) = nullptr;
    IDirect3DDevice8** device = nullptr;
    D3DPRESENT_PARAMETERS* presentParameters = nullptr;
    std::vector<uintptr_t> beforeCreateDevice, afterCreateDevice;
    std::vector<uintptr_t> beforeReset;
    std::vector<ResetCompletion> resetCompletions;
    uintptr_t beforePresent = 0;
    uintptr_t menuResolutionCall = 0;
    uintptr_t changeVideoModeCall = 0;
    uintptr_t resolutionColor = 0;
    uintptr_t resolutionEnable = 0;
    uintptr_t deviceDialog = 0;
    uintptr_t mouseRecenter = 0;
};

static_assert(sizeof(DisplayMode) == 20);
static_assert(offsetof(RwCamera, frameBuffer) == 0x60);
static_assert(sizeof(D3DPRESENT_PARAMETERS) == 52);
static_assert(sizeof(D3DPRESENT_PARAMETERS_D3D9) == 56);

template <size_t count = 1, typename... Args>
hook::pattern find_pattern(Args... args)
{
    hook::pattern pattern;
    ((pattern = hook::pattern(args), !pattern.count_hint(count).empty()) || ...);
    return pattern;
}

template<class T> inline T ReadOperand(const void* instruction, ptrdiff_t offset)
{
    return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(instruction) + offset);
}
template<class T> inline T ReadOperand(uintptr_t instruction, ptrdiff_t offset)
{
    return *reinterpret_cast<T*>(instruction + offset);
}

inline uintptr_t CallTarget(uintptr_t call)
{
    if (*reinterpret_cast<uint8_t*>(call) != 0xE8)
        throw std::runtime_error("Expected a relative call at the resolved hook site.");
    return call + 5 + ReadOperand<int32_t>(call, 1);
}

inline void ResolveD3DCalls(GameBindings& game)
{
    auto collect = [](std::string_view bytes, size_t count, ptrdiff_t before, ptrdiff_t after,
        std::vector<uintptr_t>& beforeCalls, std::vector<uintptr_t>& afterCalls, size_t afterEvery = 1)
    {
        auto pattern = hook::pattern(bytes);
        if (pattern.size() != count)
            throw std::runtime_error("Missing or ambiguous RenderWare calls: " + std::string(bytes));
        for (size_t i = 0; i < count; ++i)
        {
            auto address = reinterpret_cast<uintptr_t>(pattern.get(i).get<void>());
            beforeCalls.push_back(address + before);
            if ((i + 1) % afterEvery == 0) afterCalls.push_back(address + after);
        }
        return reinterpret_cast<uintptr_t>(pattern.get(0).get<void>());
    };
    std::vector<uintptr_t> afterReset;
    const bool sa = game.title == GameTitle::GTASA;
    const bool alternate = game.title == GameTitle::GTA3 &&
        !hook::pattern("C7 44 24 10 ? ? ? ? 8B 12 FF 52 3C").empty();
    uintptr_t reset;
    if (alternate)
    {
        // Each group of three CreateDevice attempts joins after its last call.
        // Hooking the earlier short jumps would overwrite another branch's entry.
        auto create = collect("C7 44 24 10 ? ? ? ? 8B 12 FF 52 3C", 6, 8, 13,
            game.beforeCreateDevice, game.afterCreateDevice, 3);
        reset = collect("8B 10 89 04 24 FF 52 38", 8, 0, 8,
            game.beforeReset, afterReset);
        game.presentParameters = ReadOperand<D3DPRESENT_PARAMETERS*>(create, -8);
        game.device = ReadOperand<IDirect3DDevice8**>(create, -19);
        auto present = hook::pattern("8B 02 89 14 24 FF 50 3C 8B D8");
        if (present.size() != 1)
            throw std::runtime_error("Missing or ambiguous executable pattern.");
        game.beforePresent = reinterpret_cast<uintptr_t>(present.get_first());
    }
    else if (sa)
    {
        collect("50 51 FF 52 40 85 C0 7D", 1, 0, 5, game.beforeCreateDevice, game.afterCreateDevice);
        reset = collect("A1 ? ? ? ? ? ? ? ? ? 50 8B ? FF ? 40", 10, 10, 16, game.beforeReset, afterReset);
        auto parameters = collect("68 ? ? ? ? 8B ? 50 FF ? 40", 2, 5, 11, game.beforeReset, afterReset);
        collect("8B ? 68 ? ? ? ? 50 FF ? 40", 4, 2, 11, game.beforeReset, afterReset);
        game.presentParameters = ReadOperand<D3DPRESENT_PARAMETERS*>(parameters, 1);
        game.device = ReadOperand<IDirect3DDevice8**>(reset, 1);
        auto present = hook::pattern("50 8B 10 FF 52 44 8B D8 81 FB 68 08 76 88");
        if (present.size() != 1)
            throw std::runtime_error("Missing or ambiguous executable pattern.");
        game.beforePresent = reinterpret_cast<uintptr_t>(present.get_first());
    }
    else
    {
        collect("52 50 FF 51 3C 3B C5 7D", 1, 0, 5, game.beforeCreateDevice, game.afterCreateDevice);
        reset = collect("A1 ? ? ? ? ? ? ? ? ? 50 8B ? FF ? 38", game.title == GameTitle::GTA3 ? 8 : 6,
            10, 16, game.beforeReset, afterReset);
        auto parameters = collect("68 ? ? ? ? 8B ? 50 FF ? 38", 2, 5, 11, game.beforeReset, afterReset);
        game.presentParameters = ReadOperand<D3DPRESENT_PARAMETERS*>(parameters, 1);
        game.device = ReadOperand<IDirect3DDevice8**>(reset, 1);
        auto present = hook::pattern("50 8B ? FF ? 3C 3D 68 08 76 88");
        if (present.size() != 1)
            throw std::runtime_error("Missing or ambiguous executable pattern.");
        game.beforePresent = reinterpret_cast<uintptr_t>(present.get_first());
    }

    for (auto address : afterReset)
    {
        auto code = reinterpret_cast<const uint8_t*>(address);
        auto result = ResetResult::Eax;
        // Retry paths join just after the instruction saving HRESULT. Leave
        // that branch target intact, and read the saved result at the join.
        if (code[0] == 0x89 && code[1] == 0x44 && code[2] == 0x24 && code[3] == 0x10)
        {
            address += 4;
            result = ResetResult::Stack16;
        }
        else if (code[0] == 0x8B && (code[1] == 0xD8 || code[1] == 0xF0))
        {
            address += 2;
            result = code[1] == 0xD8 ? ResetResult::Ebx : ResetResult::Esi;
        }
        game.resetCompletions.push_back({ address, result });
    }

}

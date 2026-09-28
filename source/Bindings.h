#pragma once
#include "stdafx.h"

enum class GameTitle { GTA3, GTAVC, GTASA };

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

// Resolve every binding before installing any game-code patches. Never accept
// an ambiguous signature, including when a different mod changed the code.
inline uintptr_t FindPattern(std::string_view bytes)
{
    auto pattern = hook::pattern(bytes);
    if (pattern.size() != 1)
        throw std::runtime_error("Missing or ambiguous executable pattern: " + std::string(bytes));
    return reinterpret_cast<uintptr_t>(pattern.get_first());
}

inline uintptr_t FindPattern(std::string_view first, std::string_view fallback)
{
    auto pattern = hook::pattern(first);
    if (pattern.empty()) return FindPattern(fallback);
    if (pattern.size() != 1) throw std::runtime_error("Ambiguous executable pattern: " + std::string(first));
    return reinterpret_cast<uintptr_t>(pattern.get_first());
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

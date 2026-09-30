#include "stdafx.h"
#include <atomic>

import Game;

namespace
{
    constexpr auto windowCreationPattern = "68 00 00 CF 00 ? ? 6A 00 FF 15 ? ? ? ?";
    SafetyHookInline startupHook;
    std::atomic_bool initializationPending = true;

    void Initialize()
    {
        if (!initializationPending.exchange(false)) return;
        try { InitializeGame(); }
        catch (const std::exception& error)
        {
            ShowError("This executable is unsupported or its required code has been modified.\n%s", error.what());
        }
    }

    void WINAPI StartupHook(LPFILETIME time)
    {
        startupHook.unsafe_stdcall<void>(time);
        // As in Widescreen Fix, wait for packed executable code to be available.
        if (initializationPending && !hook::pattern(windowCreationPattern).empty())
            Initialize();
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        if (!hook::pattern(windowCreationPattern).empty())
            Initialize();
        else
        {
            startupHook = safetyhook::create_inline(GetSystemTimeAsFileTime, StartupHook);
            if (!startupHook) ShowError("Could not defer initialization until executable unpacking.");
        }
    });
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        InitializeASI();
    }
    return TRUE;
}

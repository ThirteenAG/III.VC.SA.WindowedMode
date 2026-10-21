#include "stdafx.h"
#include "Settings.h"
#include <callbacks.h>

import Game;

ASI_SETTINGS_EXPORT(Settings())

namespace
{
    constexpr auto windowCreationPattern = "68 00 00 CF 00 ? ? 6A 00 FF 15 ? ? ? ?";

    // Window sizes and positions in physical pixels at any Windows scaling
    void SetDpiAwareness()
    {
        using SetContextFn = BOOL(WINAPI*)(HANDLE);
        auto user32 = GetModuleHandleW(L"user32.dll");
        auto setContext = reinterpret_cast<SetContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (!setContext || !setContext(reinterpret_cast<HANDLE>(-4))) // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            SetProcessDPIAware();
    }

    void Initialize()
    {
        try
        {
            InitializeGame();
        }
        catch (const std::exception& error)
        {
            ShowError("This executable is unsupported or its required code has been modified.\n%s", error.what());
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        Settings();
        asi_settings::LiveLink::Start();
        SetDpiAwareness();
        // Steam executables are unpacked later: wait for their code
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Initialize, hook::pattern(windowCreationPattern));
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

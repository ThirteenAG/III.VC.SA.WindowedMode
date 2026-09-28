#include "stdafx.h"

import Hooks;

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        try { InstallHooks(); }
        catch (const std::exception& error) { ShowError("%s", error.what()); }
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

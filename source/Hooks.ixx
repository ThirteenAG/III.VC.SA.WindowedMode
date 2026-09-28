module;
#include "stdafx.h"
#include "Bindings.h"

export module Hooks;

import Game;
import WindowedMode;

namespace
{
    SafetyHookInline shCreateWindow;
    SafetyHookInline shDirect3DCreate8;
    SafetyHookInline shDirect3DCreate9;
    using CreateDevice = HRESULT(WINAPI*)(void*, UINT, DWORD, HWND, DWORD, void*, IDirect3DDevice8**);
    std::map<uintptr_t*, CreateDevice> deviceFactories;
    bool initializationFailed = false;

    bool Initialize(bool d3d9)
    {
        if (initializationFailed) return false;
        try
        {
            InitializeGame(d3d9);
            return true;
        }
        catch (const std::exception& error)
        {
            initializationFailed = true;
            ShowError("This executable is unsupported or its required code has been modified.\n%s", error.what());
            return false;
        }
    }

    template<bool D3D9>
    HRESULT WINAPI CreateDeviceHook(void* self, UINT adapter, DWORD type, HWND window,
        DWORD behavior, void* parameters, IDirect3DDevice8** device)
    {
        auto original = deviceFactories.at(*reinterpret_cast<uintptr_t**>(self));
        const bool handle = inst && inst->window && inst->IsD3D9() == D3D9 && window == inst->window && parameters;
        if (handle)
        {
            inst->d3dPresentParams8 = static_cast<D3DPRESENT_PARAMETERS*>(parameters);
            inst->InitPresentationParameters();
        }
        const auto result = original(self, adapter, type, window, behavior, parameters, device);
        if (handle && SUCCEEDED(result) && device && *device)
        {
            inst->d3dDevice = *device;
            inst->InitD3dDevice();
        }
        return result;
    }

    template<bool D3D9>
    void HookFactory(void* factory)
    {
        if (!factory) return;
        auto table = *static_cast<uintptr_t**>(factory);
        if (deviceFactories.contains(table)) return;
        constexpr size_t index = D3D9 ? 16 : 15;
        DWORD protection;
        if (!VirtualProtect(table + index, sizeof(uintptr_t), PAGE_READWRITE, &protection)) return;
        deviceFactories.emplace(table, reinterpret_cast<CreateDevice>(table[index]));
        table[index] = reinterpret_cast<uintptr_t>(&CreateDeviceHook<D3D9>);
        VirtualProtect(table + index, sizeof(uintptr_t), protection, &protection);
    }

    void* WINAPI Direct3DCreate8Hook(UINT sdk)
    {
        auto factory = shDirect3DCreate8.unsafe_stdcall<void*>(sdk);
        HookFactory<false>(factory);
        return factory;
    }

    void* WINAPI Direct3DCreate9Hook(UINT sdk)
    {
        auto factory = shDirect3DCreate9.unsafe_stdcall<void*>(sdk);
        HookFactory<true>(factory);
        return factory;
    }

    HWND WINAPI CreateWindowHook(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE module, LPVOID param)
    {
        const bool named = reinterpret_cast<uintptr_t>(className) > 0xFFFF;
        const bool sa = named && !_stricmp(className, "Grand theft auto San Andreas");
        const bool classic = named && !_stricmp(className, "Grand theft auto 3");
        if (!parent && (sa || classic) && (!inst || !inst->window) && Initialize(sa))
        {
            WindowedMode::createWindow = shCreateWindow.original<decltype(&CreateWindowExA)>();
            return WindowedMode::InitWindow(exStyle, className, title, style, x, y, width, height,
                parent, menu, module, param);
        }
        return shCreateWindow.unsafe_stdcall<HWND>(exStyle, className, title, style, x, y, width, height,
            parent, menu, module, param);
    }
}

export void InstallHooks()
{
    // Win32/COM boundaries have the same calling conventions in every supported
    // executable, including III's alternate RenderWare build. Resolve game code
    // when the game creates its window, after executable unpacking.
    if (auto module = GetModuleHandleA("d3d8.dll"))
        shDirect3DCreate8 = safetyhook::create_inline(GetProcAddress(module, "Direct3DCreate8"), Direct3DCreate8Hook);
    if (auto module = GetModuleHandleA("d3d9.dll"))
        shDirect3DCreate9 = safetyhook::create_inline(GetProcAddress(module, "Direct3DCreate9"), Direct3DCreate9Hook);
    shCreateWindow = safetyhook::create_inline(CreateWindowExA, CreateWindowHook);
    if (!shCreateWindow || (!shDirect3DCreate8 && !shDirect3DCreate9))
    {
        shCreateWindow.reset();
        shDirect3DCreate8.reset();
        shDirect3DCreate9.reset();
        throw std::runtime_error("Could not install the window and Direct3D creation hooks.");
    }
}

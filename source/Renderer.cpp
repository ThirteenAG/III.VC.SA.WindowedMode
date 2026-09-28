module;
#include "stdafx.h"
#include "Bindings.h"

module WindowedMode;

namespace
{
    struct DeviceMethods
    {
        decltype(WindowedMode::D3dResetHook)* reset;
        decltype(WindowedMode::D3dPresentHook)* present;
    };
    std::map<uintptr_t*, DeviceMethods> deviceMethods;
}

void WindowedMode::InitD3dDevice()
{
    if (!d3dDevice) return;
    auto table = *reinterpret_cast<uintptr_t**>(d3dDevice);
    if (deviceMethods.contains(table)) return;

    const auto resetIndex = IsD3D9() ? 16 : 14;
    DWORD protection;
    if (!VirtualProtect(table + resetIndex, 2 * sizeof(uintptr_t), PAGE_READWRITE, &protection)) return;
    deviceMethods.emplace(table, DeviceMethods{
        reinterpret_cast<decltype(D3dResetHook)*>(table[resetIndex]),
        reinterpret_cast<decltype(D3dPresentHook)*>(table[resetIndex + 1]) });
    table[resetIndex] = reinterpret_cast<uintptr_t>(&D3dResetHook);
    table[resetIndex + 1] = reinterpret_cast<uintptr_t>(&D3dPresentHook);
    VirtualProtect(table + resetIndex, 2 * sizeof(uintptr_t), protection, &protection);
}

void WindowedMode::InitPresentationParameters()
{
    WindowCalculateGeometry();
    if (IsD3D9())
    {
        d3dPresentParams9->Windowed = TRUE;
        d3dPresentParams9->hDeviceWindow = window;
        d3dPresentParams9->BackBufferWidth = windowSizeClient.x;
        d3dPresentParams9->BackBufferHeight = windowSizeClient.y;
        d3dPresentParams9->BackBufferFormat = D3DFMT_A8R8G8B8;
        d3dPresentParams9->SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dPresentParams9->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
        d3dPresentParams9->FullScreen_RefreshRateInHz = 0;
    }
    else
    {
        d3dPresentParams8->Windowed = TRUE;
        d3dPresentParams8->hDeviceWindow = window;
        d3dPresentParams8->BackBufferWidth = windowSizeClient.x;
        d3dPresentParams8->BackBufferHeight = windowSizeClient.y;
        d3dPresentParams8->BackBufferFormat = D3DFMT_X8R8G8B8;
        d3dPresentParams8->SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dPresentParams8->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
        d3dPresentParams8->FullScreen_RefreshRateInHz = 0;
    }

    // Only adapt the startup mode. Keep the menu's original resolutions intact.
    BackupVideoModes();
    const auto current = RwEngineGetCurrentVideoMode();
    if (*rwVideoModes && current < videoModesBackup.size())
    {
        if (modifiedVideoMode >= 0 && modifiedVideoMode < (int)videoModesBackup.size())
            (*rwVideoModes)[modifiedVideoMode] = videoModesBackup[modifiedVideoMode];
        modifiedVideoMode = (int)current;
        auto& mode = (*rwVideoModes)[current];
        mode.width = windowSizeClient.x;
        mode.height = windowSizeClient.y;
        mode.format = IsD3D9() ? d3dPresentParams9->BackBufferFormat : d3dPresentParams8->BackBufferFormat;
        mode.refreshRate = IsD3D9() ? d3dPresentParams9->FullScreen_RefreshRateInHz : d3dPresentParams8->FullScreen_RefreshRateInHz;
        mode.flags &= ~1; // clear fullscreen flag
    }
}

bool WindowedMode::IsD3D9() const
{
    return gameTitle == GameTitle::GTASA;
}

HRESULT WindowedMode::D3dPresentHook(IDirect3DDevice8* self, const RECT* srcRect, const RECT* dstRect, HWND wnd, const RGNDATA* region)
{
    auto original = deviceMethods.at(*reinterpret_cast<uintptr_t**>(self)).present;
    if (!inst || self != inst->d3dDevice)
        return original(self, srcRect, dstRect, wnd, region);
    // Reset returns before RW restores its rasters. Present is the first hooked
    // point at which those resources and the camera dimensions are valid again.
    if (inst->postEffectUpdatePending && SUCCEEDED(self->TestCooperativeLevel()))
    {
        inst->postEffectUpdatePending = false;
        inst->UpdatePostEffect();
    }
    inst->MouseUpdate();

    if (inst->fpsCounter.update())
        inst->WindowUpdateTitle();

    return original(self, srcRect, dstRect, wnd, region);
}

HRESULT WindowedMode::D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters)
{
    auto original = deviceMethods.at(*reinterpret_cast<uintptr_t**>(self)).reset;
    if (!inst || self != inst->d3dDevice) return original(self, parameters);
    if (!parameters || inst->resetInProgress) return D3DERR_INVALIDCALL;
    inst->resetInProgress = true;
    inst->postEffectUpdatePending = false;
    // Preserve the caller's structure, including AA/depth settings and rollback
    // sizes. D3D8 and D3D9 have different layouts after MultiSampleType.
    // Zero dimensions mean the current client size, not a menu request.
    auto reset = [self, original](auto& caller) {
        auto effective = caller;
        RECT client = {};
        GetClientRect(inst->window, &client);
        if (!effective.BackBufferWidth) effective.BackBufferWidth = client.right;
        if (!effective.BackBufferHeight) effective.BackBufferHeight = client.bottom;
        if (!effective.BackBufferWidth || !effective.BackBufferHeight) return D3DERR_INVALIDCALL;
        effective.Windowed = TRUE;
        effective.hDeviceWindow = inst->window;
        effective.SwapEffect = D3DSWAPEFFECT_DISCARD;
        effective.FullScreen_RefreshRateInHz = 0;
        if (!inst->IsD3D9()) effective.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
        // Some runtimes clear the dimensions/count/format in this in/out argument.
        // RW reads them after Reset, so retain the effective values on success.
        auto driverParameters = effective;
        const auto result = original(self, reinterpret_cast<D3DPRESENT_PARAMETERS*>(&driverParameters));
        if (SUCCEEDED(result)) caller = effective;
        return result;
    };
    const auto result = inst->IsD3D9()
        ? reset(*reinterpret_cast<D3DPRESENT_PARAMETERS_D3D9*>(parameters))
        : reset(*parameters);
    inst->resetInProgress = false;
    inst->postEffectUpdatePending = SUCCEEDED(result);
    return result;
}

void WindowedMode::UpdatePostEffect()
{
    if (auto camera = *bindings.camera; camera && camera->frameBuffer)
    {
        if (bindings.blurOpen) bindings.blurOpen(camera);
        else bindings.setupBackBuffer();
    }
    UpdateWidescreenFix();
}

void WindowedMode::UpdateWidescreenFix()
{
    // Retry discovery: ASI load order is not fixed.
    const char* name = gameTitle == GTA_3 ? "GTA3.WidescreenFix.asi" :
        gameTitle == GTA_VC ? "GTAVC.WidescreenFix.asi" : "GTASA.WidescreenFix.asi";
    if (auto module = GetModuleHandleA(name))
        if (auto update = reinterpret_cast<void(__cdecl*)()>(GetProcAddress(module, "UpdateVars")))
            update();
}

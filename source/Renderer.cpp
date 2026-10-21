module;
#include "stdafx.h"
#include "Bindings.h"

module WindowedMode;

void WindowedMode::InstallRenderHooks()
{
    static std::vector<SafetyHookMid> hooks;
    if (!hooks.empty()) return;
    std::vector<SafetyHookMid> pending;
    auto add = [&pending](uintptr_t address, safetyhook::MidHookFn callback)
    {
        auto hook = safetyhook::create_mid(reinterpret_cast<void*>(address), callback);
        if (!hook) throw std::runtime_error("Could not hook a RenderWare device call.");
        pending.push_back(std::move(hook));
    };
    for (auto address : bindings.beforeCreateDevice)
        add(address, [](SafetyHookContext&) { inst->InitPresentationParameters(); });
    for (auto address : bindings.afterCreateDevice)
        add(address, [](SafetyHookContext& regs)
        {
            if (SUCCEEDED(static_cast<HRESULT>(regs.eax)))
            {
                inst->d3dDevice = *inst->bindings.device;
                inst->postEffectUpdatePending = false;
            }
        });
    for (auto address : bindings.beforeReset)
        add(address, [](SafetyHookContext&) { inst->BeforeReset(); });
    for (auto completion : bindings.resetCompletions)
    {
        switch (completion.result)
        {
        case ResetResult::Eax:
            add(completion.address, [](SafetyHookContext& regs) { inst->AfterReset(static_cast<HRESULT>(regs.eax)); });
            break;
        case ResetResult::Ebx:
            add(completion.address, [](SafetyHookContext& regs) { inst->AfterReset(static_cast<HRESULT>(regs.ebx)); });
            break;
        case ResetResult::Esi:
            add(completion.address, [](SafetyHookContext& regs) { inst->AfterReset(static_cast<HRESULT>(regs.esi)); });
            break;
        case ResetResult::Stack16:
            add(completion.address, [](SafetyHookContext& regs) { inst->AfterReset(*reinterpret_cast<HRESULT*>(regs.esp + 16)); });
            break;
        }
    }
    add(bindings.beforePresent, [](SafetyHookContext&) { inst->BeforePresent(); });
    hooks = std::move(pending);
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

void WindowedMode::BeforePresent()
{
    // RenderWare has restored its rasters before reaching this call site.
    if (postEffectUpdatePending && d3dDevice && SUCCEEDED(d3dDevice->TestCooperativeLevel()))
    {
        postEffectUpdatePending = false;
        UpdatePostEffect();
    }
    MouseUpdate();
    if (fpsCounter.update()) WindowUpdateTitle();
}

void WindowedMode::BeforeReset()
{
    resetInProgress = true;
    postEffectUpdatePending = false;
    const auto size = IsD3D9() ? sizeof(D3DPRESENT_PARAMETERS_D3D9) : sizeof(D3DPRESENT_PARAMETERS);
    memcpy(resetSaved.data(), d3dPresentParams8, size);
    auto prepare = [this](auto& parameters)
    {
        RECT client{};
        GetClientRect(window, &client);
        if (!parameters.BackBufferWidth) parameters.BackBufferWidth = client.right;
        if (!parameters.BackBufferHeight) parameters.BackBufferHeight = client.bottom;
        parameters.Windowed = TRUE;
        parameters.hDeviceWindow = window;
        parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        parameters.FullScreen_RefreshRateInHz = 0;
        if (!IsD3D9()) parameters.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    };
    if (IsD3D9()) prepare(*d3dPresentParams9);
    else prepare(*d3dPresentParams8);
    memcpy(resetEffective.data(), d3dPresentParams8, size);
}

void WindowedMode::AfterReset(HRESULT result)
{
    if (!resetInProgress) return;
    // Some drivers clear in/out dimensions. Keep effective values on success,
    // and restore the original request on failure for RenderWare's retry path.
    const auto size = IsD3D9() ? sizeof(D3DPRESENT_PARAMETERS_D3D9) : sizeof(D3DPRESENT_PARAMETERS);
    memcpy(d3dPresentParams8, SUCCEEDED(result) ? resetEffective.data() : resetSaved.data(), size);
    resetInProgress = false;
    postEffectUpdatePending = SUCCEEDED(result);
}

void WindowedMode::UpdatePostEffect()
{
    if (auto camera = *bindings.camera; camera && camera->frameBuffer)
    {
        if (bindings.blurOpen) bindings.blurOpen(camera);
        else bindings.setupBackBuffer();
    }
}

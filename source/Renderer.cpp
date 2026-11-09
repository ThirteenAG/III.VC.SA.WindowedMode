module;
#include "stdafx.h"
#include "Bindings.h"

module WindowedMode;

namespace
{
    // San Andreas uses Direct3D 9 while the project builds against the Direct3D 8
    // headers: its few calls here go through their vtable slots
    enum Slot : size_t
    {
        Release = 2,
        SurfaceGetDesc = 12,
        SurfaceLockRect = 13,
        SurfaceUnlockRect = 14,
        DeviceGetBackBuffer = 18,
        DeviceCreateRenderTarget = 28,
        DeviceGetRenderTargetData = 32,
        DeviceStretchRect = 34,
        DeviceCreateOffscreenPlainSurface = 36,
    };

    template <class Result = HRESULT, class... Args>
    Result Call(void* object, Slot slot, Args... args)
    {
        using Method = Result(__stdcall*)(void*, Args...);
        return (*reinterpret_cast<Method**>(object))[slot](object, args...);
    }

    struct ComRelease
    {
        void operator()(void* object) const { Call<ULONG>(object, Release); }
    };
    using Surface = std::unique_ptr<void, ComRelease>;

    struct SurfaceDesc9
    {
        D3DFORMAT format;
        D3DRESOURCETYPE type;
        DWORD usage;
        D3DPOOL pool;
        D3DMULTISAMPLE_TYPE multiSampleType;
        DWORD multiSampleQuality;
        UINT width;
        UINT height;
    };

    // Copies the picture drawn so far this frame into a 32-bit RGBA image of the same size
    bool CopyBackBuffer(void* device, void* backBuffer, const SurfaceDesc9& desc, RwImage* image)
    {
        const bool direct = desc.multiSampleType == D3DMULTISAMPLE_NONE &&
            (desc.format == D3DFMT_A8R8G8B8 || desc.format == D3DFMT_X8R8G8B8);
        const auto format = direct ? desc.format : D3DFMT_X8R8G8B8;
        void* source = backBuffer;
        void* raw = nullptr;
        Surface resolved;
        if (!direct)
        {
            // Resolve antialiasing and other formats into a plain 32-bit surface first
            if (FAILED(Call(device, DeviceCreateRenderTarget, desc.width, desc.height, format, D3DMULTISAMPLE_NONE,
                DWORD{ 0 }, BOOL{ FALSE }, &raw, static_cast<HANDLE*>(nullptr))))
                return false;
            resolved.reset(raw);
            if (FAILED(Call(device, DeviceStretchRect, backBuffer, static_cast<const RECT*>(nullptr), raw,
                static_cast<const RECT*>(nullptr), D3DTEXF_NONE)))
                return false;
            source = raw;
        }
        raw = nullptr;
        if (FAILED(Call(device, DeviceCreateOffscreenPlainSurface, desc.width, desc.height, format, D3DPOOL_SYSTEMMEM,
            &raw, static_cast<HANDLE*>(nullptr))))
            return false;
        Surface copy(raw);
        if (FAILED(Call(device, DeviceGetRenderTargetData, source, raw)))
            return false;
        D3DLOCKED_RECT locked{};
        if (FAILED(Call(raw, SurfaceLockRect, &locked, static_cast<const RECT*>(nullptr), DWORD{ D3DLOCK_READONLY })))
            return false;
        for (int32_t y = 0; y < image->height; ++y)
        {
            auto src = static_cast<const uint8_t*>(locked.pBits) + y * locked.Pitch;
            auto dst = image->cpPixels + y * image->stride;
            for (int32_t x = 0; x < image->width; ++x, src += 4, dst += 4)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst[3] = 0xFF;
            }
        }
        Call(raw, SurfaceUnlockRect);
        return true;
    }
}

// The game grabs photos and screenshots from the front buffer. In a window that
// is the whole desktop, read at the window's outer rectangle without clipping:
// with the frame off the screen, or on another monitor, that reads outside the
// copy. The back buffer holds exactly the game's picture in every window mode.
RwImage* __cdecl WindowedMode::GrabScreenSA()
{
    void* device = *inst->bindings.device;
    Surface backBuffer;
    SurfaceDesc9 desc{};
    void* raw = nullptr;
    if (device && SUCCEEDED(Call(device, DeviceGetBackBuffer, UINT{ 0 }, UINT{ 0 }, D3DBACKBUFFER_TYPE_MONO, &raw)))
    {
        backBuffer.reset(raw);
        if (FAILED(Call(raw, SurfaceGetDesc, &desc)))
            desc = {};
    }
    const auto width = desc.width ? int32_t(desc.width) : int32_t(inst->windowSizeClient.x);
    const auto height = desc.height ? int32_t(desc.height) : int32_t(inst->windowSizeClient.y);

    auto image = inst->bindings.imageCreate((std::max)(width, 1), (std::max)(height, 1), 32);
    if (!image)
        return nullptr;
    if (!inst->bindings.imageAllocatePixels(image) || !image->cpPixels)
        return image;
    // The game saves the image unchecked: a failed read leaves it black
    if (!desc.width || !CopyBackBuffer(device, backBuffer.get(), desc, image))
    {
        for (int32_t y = 0; y < image->height; ++y)
        {
            auto dst = reinterpret_cast<uint32_t*>(image->cpPixels + y * image->stride);
            std::fill_n(dst, image->width, 0xFF000000u);
        }
    }
    return image;
}

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

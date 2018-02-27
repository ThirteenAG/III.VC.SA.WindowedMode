#include "dxhandler.h"

void CDxHandler::SetupHooksVC(void)
{
    CMBlurMotionBlurOpen = (void(*)(RwCamera*))0x55CE20;
    DxInputGetMouseState = (int(*)(int))0x5FF290;
    ReinitializeRw = (void(*)(int))0x601770;
    RwEngineGetCurrentVideoMode = (int(*)())0x642BA0;
    RwCameraClear = (RwCamera*(*)(RwCamera*, void*, int32_t))0x64A9D0;
    RwRasterDestroy = (bool(*)(RwRaster*))0x6552E0;
    RwRasterCreate = (RwRaster*(*)(int32_t, int32_t, int32_t, int32_t))0x655490;

    pRenderCamera = (RwCamera**)0x8100BC;
    pIntDirect3DMain = (IDirect3D8**)0x7897B4;
    pDirect3DDevice = (IDirect3DDevice8**)0x7897A8;
    pInputData = (GameDxInput**)0x9B48F4;
    bMenuVisible = (bool*)0x869668;
    hGameWnd = (HWND*)0x7897A4;
    pDisplayModes = (DisplayMode**)0x7897D0;
    RwD3D8AdapterInformation_DisplayMode = 0x9B6CBC;
    CamCol = 0x983B80;
    HookParams = 0xA0FD04;
    bBlurOn = (bool*)0x697D54;
    RsGlobal = (RsGlobalType*)0x9B48D8;
    WSFixHook = 0x60041D;

    injector::MakeJMP(0x65C3AC, HookDirect3DDeviceReplacer, true);
    HookDirect3DDeviceReplacerJmp = 0x65C3B1;
    Direct3DDeviceReplace();
    InjectWindowProc();

    struct HookDxInputCreateDevice
    {
        void operator()(injector::reg_pack& regs)
        {
            static bool bExclusiveMode;

            regs.eax = *(uintptr_t*)(regs.esp + 0x4);
            bExclusiveMode = LOBYTE(regs.eax);

            CDxHandler::DxInputCreateDevice(bExclusiveMode);

            *(uintptr_t*)regs.esp = 0x5FFAA3;
        }
    }; injector::MakeInline<HookDxInputCreateDevice>(0x5FFA20);


    struct HookDxMouseUpdater
    {
        void operator()(injector::reg_pack& regs)
        {
            if (CDxHandler::ProcessMouseState() == 0)
                *(uintptr_t*)regs.esp = 0x4ADA18;

            DxInputGetMouseState(regs.eax);

        }
    }; injector::MakeInline<HookDxMouseUpdater>(0x4AD86D);


    struct HookDxCameraClearFix
    {
        void operator()(injector::reg_pack& regs)
        {
            static RwCamera* pCamera;
            regs.ebp = *(uintptr_t*)(regs.esp + 0x7C); //regs.ebp is 0 sometimes, so it crashes, but with asm it works for some reason

            pCamera = (RwCamera*)regs.ebp;
            CDxHandler::MainCameraRebuildRaster(pCamera);

            regs.esi = *(uintptr_t*)(regs.ebp + 0x60);
        }
    }; injector::MakeInline<HookDxCameraClearFix>(0x65D46D, 0x65D474);

    injector::MakeNOP(0x6011C0, 16, true);

    struct HookDxReload
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uintptr_t*)regs.esp = 0x6004E7;

            bStopRecursion = true;

            ReinitializeRw(RwEngineGetCurrentVideoMode());
            RwCameraClear(*pRenderCamera, (void*)CamCol, 2);

            bStopRecursion = false;
        }
    }; injector::MakeInline<HookDxReload>(0x600357);

    struct HookResChange
    {
        void operator()(injector::reg_pack& regs)
        {
            static int32_t nDefaultVideoMode = -1;
            static uint32_t nDefaultVideoModeResX = 0;
            static uint32_t nDefaultVideoModeResY = 0;

            *(uintptr_t*)(regs.esp - 4) = 0x499ABC;

            if (nDefaultVideoMode == -1)
            {
                int nPrevModeIndex = *(uintptr_t*)(regs.ebp + 0x5C);
                nDefaultVideoMode = nPrevModeIndex;
                nDefaultVideoModeResX = (*CDxHandler::pDisplayModes)[nPrevModeIndex].nWidth;
                nDefaultVideoModeResY = (*CDxHandler::pDisplayModes)[nPrevModeIndex].nHeight;
            }

            *(uintptr_t*)(regs.ebp + 0x5C) = regs.eax;

            int nModeIndex = *(uint32_t*)(regs.ebp + 0x5C);

            if (nModeIndex == nDefaultVideoMode)
            {
                ((D3DPRESENT_PARAMETERS*)HookParams)->BackBufferWidth = nDefaultVideoModeResX;
                ((D3DPRESENT_PARAMETERS*)HookParams)->BackBufferHeight = nDefaultVideoModeResY;
            }
            else
            {
                ((D3DPRESENT_PARAMETERS*)HookParams)->BackBufferWidth = (*CDxHandler::pDisplayModes)[nModeIndex].nWidth;
                ((D3DPRESENT_PARAMETERS*)HookParams)->BackBufferHeight = (*CDxHandler::pDisplayModes)[nModeIndex].nHeight;
            }
            CDxHandler::AdjustPresentParams((D3DPRESENT_PARAMETERS*)HookParams);
        }
    };

    struct HookResChangeJmp
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uintptr_t*)regs.esp = 0x6004E7;
            bInGame3VC = true;
            injector::MakeInline<HookResChange>(0x4999CA);
            injector::MakeNOP(0x499F57, 2, true); //menu resoluton option unblocking
            injector::MakeJMP(0x49EDBC, 0x49EF35, true);
            injector::MakeJMP(0x48F8C1, 0x48F8D4, true);

            CDxHandler::AdjustPresentParams((D3DPRESENT_PARAMETERS*)HookParams); // menu
        }
    }; injector::MakeInline<HookResChangeJmp>(0x600427);
}
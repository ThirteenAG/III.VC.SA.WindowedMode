#include "dxhandler.h"

void CDxHandler::SetupHooksIII(void)
{
    CMBlurMotionBlurOpen = (void(*)(RwCamera*))0x50AE40;
    DxInputGetMouseState = (int(*)(int))0x583870;
    ReinitializeRw = (void(*)(int))0x581630;
    RwEngineGetCurrentVideoMode = (int(*)())0x5A0F30;
    RwCameraClear = (RwCamera*(*)(RwCamera*, void*, int32_t))0x5A51E0;
    RwRasterDestroy = (bool(*)(RwRaster*))0x5AD780;
    RwRasterCreate = (RwRaster*(*)(int32_t, int32_t, int32_t, int32_t))0x5AD930;

    pRenderCamera = (RwCamera**)0x72676C;
    pIntDirect3DMain = (IDirect3D8**)0x662EFC;
    pDirect3DDevice = (IDirect3DDevice8**)0x662EF0;
    pInputData = (GameDxInput**)0x8F437C;
    bMenuVisible = (bool*)0x8F5AE9;
    hGameWnd = (HWND*)0x662EEC;
    pDisplayModes = (DisplayMode**)0x662F18;
    RwD3D8AdapterInformation_DisplayMode = 0x8F620C;
    CamCol = 0x8F4328;
    HookParams = 0x943010;
    bBlurOn = (bool*)0x95CDAD;
    RsGlobal = (RsGlobalType*)0x8F4360;

    injector::MakeJMP(0x5B7DF4, HookDirect3DDeviceReplacer, true);
    HookDirect3DDeviceReplacerJmp = 0x5B7DF9;

    Direct3DDeviceReplace();
    InjectWindowProc();

    //struct HookDxInputCreateDevice
    //{
    //	void operator()(injector::reg_pack& regs)
    //	{
    //		static bool bExclusiveMode = false;
    //
    //		CDxHandler::DxInputCreateDevice(bExclusiveMode);
    //
    //		*(uintptr_t*)(regs.esp - 4) = 0x583182;
    //	}
    //}; injector::MakeInline<HookDxInputCreateDevice>(0x583110);


    struct HookDxMouseUpdater
    {
        void operator()(injector::reg_pack& regs)
        {
            if (CDxHandler::ProcessMouseState() == 0)
                *(uintptr_t*)regs.esp = 0x583900;

            DxInputGetMouseState(regs.eax);

        }
    }; injector::MakeInline<HookDxMouseUpdater>(0x491CD6);


    struct HookDxCameraClearFix
    {
        void operator()(injector::reg_pack& regs)
        {
            static RwCamera* pCamera;
            regs.ebp = *(uintptr_t*)(regs.esp + 0x7C);

            pCamera = (RwCamera*)regs.ebp;
            CDxHandler::MainCameraRebuildRaster(pCamera);

            regs.esi = *(uintptr_t*)(regs.ebp + 0x60);
        }
    }; injector::MakeInline<HookDxCameraClearFix>(0x5B8E97, 0x5B8E9E);

    injector::MakeNOP(0x581890, 16, true);

    struct HookDxReload
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uintptr_t*)regs.esp = 0x582F3D;

            bStopRecursion = true;

            ReinitializeRw(RwEngineGetCurrentVideoMode());
            RwCameraClear(*pRenderCamera, (void*)CamCol, 2);

            bStopRecursion = false;
        }
    }; injector::MakeInline<HookDxReload>(0x582DC3);

    struct HookResChange
    {
        void operator()(injector::reg_pack& regs)
        {
            static int32_t nDefaultVideoMode = -1;
            static uint32_t nDefaultVideoModeResX = 0;
            static uint32_t nDefaultVideoModeResY = 0;

            *(uintptr_t*)(regs.esp - 4) = 0x487851;

            if (nDefaultVideoMode == -1)
            {
                int nPrevModeIndex = *(uintptr_t*)(regs.ebp);
                nDefaultVideoMode = nPrevModeIndex;
                nDefaultVideoModeResX = (*CDxHandler::pDisplayModes)[nPrevModeIndex].nWidth;
                nDefaultVideoModeResY = (*CDxHandler::pDisplayModes)[nPrevModeIndex].nHeight;
            }

            *(uintptr_t*)(regs.ebp) = regs.eax;

            int nModeIndex = *(uint32_t*)(regs.ebp);

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
            *(uintptr_t*)(regs.esp - 4) = 0x582F3D;
            bInGame3VC = true;
            injector::MakeInline<HookResChange>(0x48783B);
            injector::MakeNOP(0x4882CA, 6, true); //menu resoluton option unblocking

            CDxHandler::AdjustPresentParams((D3DPRESENT_PARAMETERS*)HookParams); // menu
        }
    }; injector::MakeInline<HookResChangeJmp>(0x582E82);
}
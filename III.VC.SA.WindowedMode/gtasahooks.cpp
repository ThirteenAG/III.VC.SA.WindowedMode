#include "dxhandler.h"
#include "includes\injector\injector.hpp"
#include "includes\injector\assembly.hpp"
#include "includes\injector\calling.hpp"

void CDxHandler::SetupHooksSA(void)
{
	CPostEffectsDoScreenModeDependentInitializations = (void(*)())0x7046D0;
	CPostEffectsSetupBackBufferVertex = (void(*)())0x7043D0;
	DxInputGetMouseState = (int(*)(int))0x746ED0;
	ReinitializeRw = (void(*)(int))0x0;
	RwEngineGetCurrentVideoMode = (int(*)())0x7F2D20;
	RwCameraClear = (RwCamera*(*)(RwCamera*, void*, int32_t))0x7EE340;
	RwRasterDestroy = (bool(*)(RwRaster*))0x7FB020;
	RwRasterCreate = (RwRaster*(*)(int32_t, int32_t, int32_t, int32_t))0x7FB230;

	pRenderCamera = (RwCamera**)0xC1703C;
	pIntDirect3DMain = (IDirect3D8**)0xC97C20;
	pDirect3DDevice = (IDirect3DDevice8**)0xC97C28;
	pInputData = (GameDxInput**)0xC17054;
	bMenuVisible = (bool*)0xBA67A4;
	hGameWnd = (HWND*)0xC97C1C;
	pDisplayModes = (DisplayMode**)0xC97C48;
	RwD3D8AdapterInformation_DisplayMode = 0xC9BEE4;
	CamCol = 0xB72CA0;
	HookParams = 0xC9C040;
	RsGlobal = (RsGlobalType*)0xC17040;

	injector::MakeJMP(0x7F6781, HookDirect3DDeviceReplacerSA, true);
	HookDirect3DDeviceReplacerJmp = 0x7F6786;

	Direct3DDeviceReplaceSA();
	InjectWindowProc();

	struct HookDxMouseUpdater
	{
		void operator()(injector::reg_pack& regs)
		{
			if (CDxHandler::ProcessMouseState() == 0)
				*(uintptr_t*)regs.esp = 0x746F60;

			DxInputGetMouseState(regs.ecx);

		}
	}; injector::MakeInline<HookDxMouseUpdater>(0x53F417);

	//struct HookDxInputCreateDevice
	//{
	//	void operator()(injector::reg_pack& regs)
	//	{
	//		static bool bExclusiveMode;
	//
	//		regs.eax = *(uintptr_t*)(regs.esp + 0x4);
	//		bExclusiveMode = LOBYTE(regs.eax);
	//
	//		CDxHandler::DxInputCreateDevice(bExclusiveMode);
	//
	//		*(uintptr_t*)(regs.esp) = 0x746A0C;
	//	}
	//}; injector::MakeInline<HookDxInputCreateDevice>(0x7469A0);

	struct HookDxCameraClearFix
	{
		void operator()(injector::reg_pack& regs)
		{
			static RwCamera* pCamera;
			regs.edx = *(uintptr_t*)(regs.esp + 0x80);

			pCamera = (RwCamera*)regs.edx;
			CDxHandler::MainCameraRebuildRaster(pCamera);
		}
	}; injector::MakeInline<HookDxCameraClearFix>(0x7F7C41, 0x7F7C41 + 7);

	injector::MakeNOP(0x7481CD, 16, true);

	struct HookDxReload
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uintptr_t*)regs.esp = 0x748DA3;
	
			bStopRecursion = true;
	
			//ReinitializeRw(RwEngineGetCurrentVideoMode());
			RwCameraClear(*pRenderCamera, (void*)CamCol, 2);
	
			bStopRecursion = false;
		}
	}; injector::MakeInline<HookDxReload>(0x748C60);

	struct HookResChangeJmp
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uintptr_t*)(regs.esp) = 0x748DA3;
			bInGameSA = true;

			CDxHandler::AdjustPresentParams((D3DPRESENT_PARAMETERS_D3D9*)HookParams); // menu
		}
	}; injector::MakeInline<HookResChangeJmp>(0x748D1A);
}
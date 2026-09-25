#include "WindowedMode.h"

void WindowedMode::InitGta3()
{
	inst = new WindowedMode(
		GameTitle::GTA_3,
		0x8F5838, // gameState
		0x8F4360, // rsGlobal
		0x662EF0, // d3dDevice
		0x943010, // d3dPresentParams
		0x662F18, // rwVideoModes
		0x5A0ED0, // RwEngineGetNumVideoModes
		0x5A0F30, // RwEngineGetCurrentVideoMode
		0x8F59D8  // FrontEndMenuManager
	);

	// check for ASI loader
	if (inst->rsGlobal->ps && inst->rsGlobal->ps->window) // app window already created
		ShowError("ASI Loader is required for correct operation of this plugin!");

	strcpy_s(inst->windowClassName, "Grand theft auto 3");
	inst->windowIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1042));

	// Debug: check if somebody already modified memory we want to hook
#ifdef DEBUG
	VerifyMemory("CreateWindow", 0x580EE5, 41, 0x8A005124);
	VerifyMemory("InitPresentationParams", 0x5B7DA1, 6, 0xB5F575C6);
	VerifyMemory("InitD3dDevice", 0x5B76B8, 6, 0x941520BC);
	VerifyMemory("Options>Resolution coloring", 0x047C6B8, 2, 0x654DDEDC);
	VerifyMemory("Options>Resolution disabling", 0x4882CA, 6, 0x66C92448);
	VerifyMemory("Options>Resolution hook", 0x487842, 5, 0x07A242DD);
#endif

	// patch call to CreateWindowExA
	injector::MakeNOP(0x580F20, 6);
	injector::MakeCALL(0x580F20, WindowedMode::InitWindow);

	struct Patch_InitPresentationParams // just before D3D device is created
	{
		void operator()(injector::reg_pack& regs)
		{
			*(DWORD*)(0x943038) = regs.ebp; // original action replaced by the patch

			inst->InitPresentationParameters();
		}
	}; injector::MakeInline<Patch_InitPresentationParams>(0x5B7DA1, 0x5B7DA7);

	struct Path_InitD3dDevice // just after D3D device has been created
	{
		void operator()(injector::reg_pack& regs)
		{
			*(DWORD*)(0x662F04) = regs.eax; // original action replaced by the patch

			if (regs.eax) // succeed
			{
				inst->InitD3dDevice();
			}
		}
	}; injector::MakeInline<Path_InitD3dDevice>(0x5B76B8);

	injector::WriteMemory(0x047C6B8, BYTE(0xEB), true); // don't gray out resoluton in options menu after game started
	injector::MakeNOP(0x4882CA, 6); // don't disable resoluton changes in options menu after game started
	struct Patch_ChangeResolution // user selected new resolution in option menu
	{
		void operator()(injector::reg_pack& regs)
		{
			inst->ChangeResolution(regs.eax);
		}
	}; injector::MakeInline<Patch_ChangeResolution>(0x487842);
}

#include "WindowedMode.h"

void WindowedMode::InitGtaSA()
{
	inst = new WindowedMode(
		GameTitle::GTA_SA,
		0xC8D4C0, // gameState
		0xC17040, // rsGlobal
		0xC97C28, // d3dDevice
		0xC9C040, // d3dPresentParams
		0xC97C48, // rwVideoModes
		0x7F2CC0, // RwEngineGetNumVideoModes
		0x7F2D20, // RwEngineGetCurrentVideoMode
		0xBA6748  // FrontEndMenuManager
	);

	strcpy_s(inst->windowClassName, "Grand theft auto San Andreas");
	inst->windowIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(100));

	// Debug: check if somebody already modified memory we want to hook
#ifdef DEBUG
	VerifyMemory("Device selection", 0x746225, 1, 0x65BFB3B6);
	VerifyMemory("RsMouseSetPos call", 0x53E9F1, 5, 0x9D112499);
	VerifyMemory("CreateWindow", 0x7455D5, 6, 0x4A88D8AA);
	VerifyMemory("InitPresentationParams", 0x7F670A, 6, 0xAB349BBF);
	VerifyMemory("InitD3dDevice", 0x7F6800, 6, 0xEDAE7102);
	VerifyMemory("Options>Resolution hook", 0x57D096, 5, 0xC41B9B94);
	VerifyMemory("ChangeVideoMode hook", 0x745C75, 5, 0x59C86E7D);
#endif

	// do not show device selection dialog in case of multiple display monitors
	injector::WriteMemory(0x746225, BYTE(0xEB), true);

	// disable recentering cursor pos. Frees mouse when window inactive but game not paused
	injector::MakeNOP(0x53E9F1, 5); // call to RsMouseSetPos

	// patch call to CreateWindowExA
	injector::MakeNOP(0x7455D5, 6);
	injector::MakeCALL(0x7455D5, WindowedMode::InitWindow);

	// Explicit menu resolution request; AA/settings changes keep the window size.
	injector::MakeCALL(0x57D096, WindowedMode::ChangeResolutionSA);
	// Keep the native RW resource release/restore path for all video-mode changes.
	injector::MakeCALL(0x745C75, WindowedMode::ChangeVideoModeSA);

	struct Patch_InitPresentationParams // just before D3D device is created
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(DWORD*)(0xC97C4C); // original action replaced by the patch

			inst->InitPresentationParameters();
		}
	}; injector::MakeInline<Patch_InitPresentationParams>(0x7F670A, 0x7F6710);

	struct Path_InitD3dDevice // just after D3D device has been created
	{
		void operator()(injector::reg_pack& regs)
		{
			*(DWORD*)(0xC9808C) = regs.ebp; // original action replaced by the patch

			inst->InitD3dDevice();
		}
	}; injector::MakeInline<Path_InitD3dDevice>(0x7F6800, 0x7F6806);
}

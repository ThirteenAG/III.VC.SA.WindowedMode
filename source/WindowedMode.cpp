#include "WindowedMode.h"
#include "Windowed_Gta3.h"
#include "Windowed_GtaVC.h"
#include "Windowed_GtaSA.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib") // DwmGetWindowAttribute
#pragma comment(lib, "winmm.lib") // timeGetTime

// list of popular display aspect ratios
const WindowedMode::AspectRatioInfo WindowedMode::AspectRatios[] = {
	{ "1:1", 1.0f / 1.0f },
	{ "2:1", 2.0f / 1.0f },
	{ "4:3", 4.0f / 3.0f },
	{ "5:4", 5.0f / 4.0f },
	{ "10:7", 10.0f / 7.0f }, // GTA default aspect
	{ "16:9", 16.0f / 9.0f },
	{ "16:10", 16.0f / 10.0f }
};

WindowedMode::WindowedMode(
	GameTitle gameTitle,
	uintptr_t gameState,
	uintptr_t rsGlobal,
	uintptr_t d3dDevice,
	uintptr_t d3dPresentParams,
	uintptr_t rwVideoModes,
	uintptr_t RwEngineGetNumVideoModes,
	uintptr_t RwEngineGetCurrentVideoMode,
	uintptr_t frontEndMenuManager
) :
	gameTitle(gameTitle),
	gameState(*(GameState*)gameState),
	rsGlobal((RsGlobalType*)rsGlobal),
	d3dDevice(*(IDirect3DDevice8**)d3dDevice),
	d3dPresentParams8((D3DPRESENT_PARAMETERS*)d3dPresentParams),
	rwVideoModes((DisplayMode**)rwVideoModes),
	RwEngineGetNumVideoModes(*(DWORD(*)())RwEngineGetNumVideoModes),
	RwEngineGetCurrentVideoMode(*(DWORD(*)())RwEngineGetCurrentVideoMode),
	frontEndMenuManager(frontEndMenuManager)
{}

HWND __stdcall WindowedMode::InitWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	WNDCLASSA oriClass;
	if (!GetClassInfo(hInstance, inst->windowClassName, &oriClass))
	{
		ShowError("Game window class not found!");
		return NULL;
	}
	inst->oriWindowProc = oriClass.lpfnWndProc;

	inst->InitConfig();
	bool maximize = inst->LoadConfig();
	bool center = (inst->windowPos.x == -1) || (inst->windowPos.y == -1);

	inst->WindowCalculateGeometry(center);
	inst->WindowUpdateTitle();

	WNDCLASSA wndClass;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = inst->windowClassName;
	wndClass.style = 0;
	wndClass.hIcon = inst->windowIcon;
	wndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wndClass.lpszMenuName = NULL;
	wndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wndClass.lpfnWndProc = &WindowedMode::WindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	
	UnregisterClass(inst->windowClassName, hInstance);
	RegisterClass(&wndClass);

	inst->window = CreateWindowEx(
		inst->WindowStyleEx(),
		inst->windowClassName,
		inst->windowTitle,
		inst->WindowStyle(),
		inst->windowPos.x, inst->windowPos.y,
		inst->windowSize.x, inst->windowSize.y,
		NULL, // parent
		NULL, // menu
		hInstance,
		0);

	if (maximize)
		PostMessage(inst->window, WM_SYSCOMMAND, SC_MAXIMIZE, 0); // maximize and perofm updates
	else if (inst->windowMode == WindowMode::Windowed)
		inst->WindowCalculateGeometry(center, true); // now modern styles border padding can be calculcated

	UpdateWindow(inst->window);
	inst->MouseUpdate(true); // lock cursor in the window until main menu appears

	return inst->window;
}

void WindowedMode::InitD3dDevice()
{
	if (d3dDevice == nullptr)
	{
		return;
	}

	auto vTable = *(uintptr_t**)d3dDevice;
	DWORD oldProtect;
	if (!injector::UnprotectMemory(vTable, 20 * sizeof(uintptr_t), oldProtect)) return;

	const auto resetIndex = IsD3D9() ? 16 : 14;
	const auto presentIndex = resetIndex + 1;
	// Device recreation can reuse a vtable that we already hooked.
	if (vTable[resetIndex] != (uintptr_t)&D3dResetHook)
	{
		d3dResetOri = reinterpret_cast<decltype(d3dResetOri)>(vTable[resetIndex]);
		vTable[resetIndex] = (uintptr_t)&D3dResetHook;
	}
	if (vTable[presentIndex] != (uintptr_t)&D3dPresentHook)
	{
		d3dPresentOri = reinterpret_cast<decltype(d3dPresentOri)>(vTable[presentIndex]);
		vTable[presentIndex] = (uintptr_t)&D3dPresentHook;
	}
	injector::ProtectMemory(vTable, 20 * sizeof(uintptr_t), oldProtect);
}

void WindowedMode::InitConfig()
{
	auto attr = GetFileAttributes(config.GetIniPath().string().c_str());
	
	if (attr == INVALID_FILE_ATTRIBUTES) // does not exists
		SaveConfig();
}

bool WindowedMode::LoadConfig()
{
	windowMode = (WindowMode)config.ReadInteger("window", "mode", WindowMode::Windowed);
	windowMode = std::clamp(windowMode, WindowMode::Min, WindowMode::Max);
	windowedStyle = (WindowMode)std::clamp(config.ReadInteger("window", "windowedStyle",
		windowMode == Fullscreen ? Windowed : windowMode), int(Windowed), int(WindowedBorderless));
	if (windowMode != Fullscreen) windowedStyle = windowMode;

	bool maximize = config.ReadInteger("window", "maximized", 0) != false;

	windowPosWindowed.x = config.ReadInteger("window", "positionX", -1);
	windowPosWindowed.y = config.ReadInteger("window", "positionY", -1);

	windowSizeWindowed.x = max(config.ReadInteger("window", "resolutionX", Resolution_Default.x), Resolution_Min.x);
	windowSizeWindowed.y = max(config.ReadInteger("window", "resolutionY", Resolution_Default.y), Resolution_Min.y);
	
	windowPos = windowPosWindowed;
	windowSize = windowSizeClient = windowSizeWindowed;
	
	autoPause = config.ReadInteger("game", "autoPause", true) != false;
	autoResume = config.ReadInteger("game", "autoResume", true) != false;

	return maximize;
}

void WindowedMode::SaveConfig()
{
	config.WriteString("window", "windowedStyle", StringPrintf("%d\t; 1: framed, 2: borderless", windowedStyle));
	config.WriteString("window", "mode",		StringPrintf("%d\t\t\t; 1: window, 2: window borderless, 3: fullscreen", windowMode));
	config.WriteString("window", "maximized",	StringPrintf("%d", IsZoomed(window)));
	config.WriteString("window", "positionX",	StringPrintf("%d\t; -1: centered", windowPosWindowed.x));
	config.WriteString("window", "positionY",	StringPrintf("%d\t; -1: centered", windowPosWindowed.y));
	config.WriteString("window", "resolutionX",	StringPrintf("%d", windowSizeWindowed.x));
	config.WriteString("window", "resolutionY",	StringPrintf("%d", windowSizeWindowed.y));
	
	config.WriteString("game", "autoPause",		StringPrintf("%d\t\t; pause the game on window deactivation", autoPause));
	config.WriteString("game", "autoResume",	StringPrintf("%d\t; resume the game on window activation", autoResume));
}

int WindowedMode::FindAspectRatio(POINT resolution, float treshold)
{
	auto ratio = float(resolution.x) / resolution.y;

	// find best match in
	int idx = -1;
	float dist = 9999.0f;
	for (size_t i = 0; i < _countof(AspectRatios); i++)
	{
		auto diff = fabs(AspectRatios[i].ratio - ratio);
		if (diff < dist)
		{
			idx = i;
			dist = diff;
		}
	}

	return dist <= treshold ? idx : -1;
}

DWORD WindowedMode::WindowStyle() const
{
	return WS_VISIBLE | WS_CLIPSIBLINGS | ((windowMode == WindowMode::Windowed) ?
		WS_OVERLAPPEDWINDOW :
		WS_POPUP);
}

DWORD WindowedMode::WindowStyleEx() const
{
	return (windowMode == WindowMode::Windowed) ?
		0 : // WS_EX_CLIENTEDGE
		0;
}

void WindowedMode::WindowCalculateGeometry(bool center, bool resizeWindow)
{
	if (windowUpdating) return;
	windowUpdating = true;

	POINT windowCenter = { windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2};
	auto monitorRect = GetMonitorRect(windowCenter);
	auto monitorWidth = monitorRect.right - monitorRect.left;
	auto monitorHeight = monitorRect.bottom - monitorRect.top;
	bool monitorSingle = GetSystemMetrics(SM_CMONITORS) <= 1;

	// size
	if (windowMode == WindowMode::Fullscreen)
	{
		windowPos.x = monitorRect.left;
		windowPos.y = monitorRect.top;
		windowSize.x = windowSizeClient.x = monitorWidth;
		windowSize.y = windowSizeClient.y = monitorHeight;
	}
	else if (!IsZoomed(window)) // not maximized windowed modes
	{
		windowSize = SizeFromClient(windowSizeWindowed);

		if (monitorSingle) // limit window size to desktop
		{
			windowSize.x = min(windowSize.x, monitorWidth);
			windowSize.y = min(windowSize.y, monitorHeight);
		}

		windowSizeClient = windowSizeWindowed = ClientFromSize(windowSize);

		// window position
		if (center)
		{
			windowPosWindowed.x = monitorRect.left + (monitorWidth - windowSize.x) / 2;
			windowPosWindowed.y = monitorRect.top + (monitorHeight - windowSize.y) / 2;
		}
		
		if (monitorSingle) // keep entire window on the screen
		{
			windowPosWindowed.x = max(windowPosWindowed.x, monitorRect.left);
			if (windowPosWindowed.x + windowSize.x > monitorRect.right)
			{
				windowPosWindowed.x = monitorRect.right - windowSize.x;
			}

			windowPosWindowed.y = max(windowPosWindowed.y, monitorRect.top);
			if (windowPosWindowed.y + windowSize.y > monitorRect.bottom)
			{
				windowPosWindowed.y = monitorRect.bottom - windowSize.y;
			}
		}

		windowPos = windowPosWindowed;
	}

	// apply to the window
	if (resizeWindow && !IsZoomed(window))
	{
		SetWindowLong(window, GWL_STYLE, WindowStyle());
		SetWindowLong(window, GWL_EXSTYLE, WindowStyleEx());
		SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW); // update the frame
		auto padding = GetFrameSize(true);

		SetWindowPos(window, 0,
			windowPos.x - padding.left,
			windowPos.y - padding.top,
			windowSize.x,
			windowSize.y,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

		WindowUpdateTitle();
	}

	// Windows is authoritative (DPI, maximization and frame metrics can change).
	RECT client;
	if (window && !IsIconic(window) && GetClientRect(window, &client) &&
		client.right > 0 && client.bottom > 0)
	{
		windowSizeClient = { client.right, client.bottom };
		if (!IsZoomed(window) && windowMode != WindowMode::Fullscreen)
			windowSizeWindowed = windowSizeClient;
	}

	// apply resolution to game internals
	if (gameTitle == GameTitle::GTA_SA)
	{
		rsGlobalSA->ps->fullScreen = false;
		rsGlobalSA->ps->window = window;
		rsGlobalSA->MaximumWidth = windowSizeClient.x;
		rsGlobalSA->MaximumHeight = windowSizeClient.y;
	}
	else
	{
		rsGlobal->ps->fullScreen = false;
		rsGlobal->ps->window = window;
		rsGlobal->screenWidth = rsGlobal->MaximumWidth = windowSizeClient.x;
		rsGlobal->screenHeight = rsGlobal->MaximumHeight = windowSizeClient.y;
	}

	// Do not edit Present here: it belongs to RenderWare's reset/recovery path.
	// In particular, WM_SIZE can be dispatched synchronously from inside Reset.
	windowUpdating = false;
	if (resizeWindow && window && d3dDevice && !resetInProgress)
		CallWindowProc(oriWindowProc, window, WM_SIZE, SIZE_RESTORED,
			MAKELPARAM(windowSizeClient.x, windowSizeClient.y));
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

void WindowedMode::BackupVideoModes()
{
	const auto count = RwEngineGetNumVideoModes();
	if (!*rwVideoModes || !count || count == DWORD(-1)) return;
	if (videoModesSource != *rwVideoModes || videoModesBackup.size() != count)
	{
		videoModesSource = *rwVideoModes;
		videoModesBackup.assign(videoModesSource, videoModesSource + count);
		modifiedVideoMode = -1;
	}
}

void WindowedMode::ChangeResolution(DWORD modeIndex)
{
	BackupVideoModes();
	if (!*rwVideoModes || modeIndex >= videoModesBackup.size()) return;
	const auto& mode = videoModesBackup[modeIndex];
	if (mode.width < UINT(Resolution_Min.x) || mode.height < UINT(Resolution_Min.y) ||
		mode.width > LONG_MAX || mode.height > LONG_MAX) return;
	WindowResize({ (LONG)mode.width, (LONG)mode.height });
}

void __cdecl WindowedMode::ChangeResolutionSA(DWORD modeIndex)
{
	inst->ChangeResolution(modeIndex);
	injector::cstd<void(DWORD)>::call(0x745C70, modeIndex);
}

int __cdecl WindowedMode::ChangeVideoModeSA(DWORD modeIndex)
{
	inst->BackupVideoModes();
	if (!*inst->rwVideoModes || modeIndex >= inst->videoModesBackup.size()) return 0;
	// SA also uses this call for AA changes and loading settings. Those are not
	// window resize requests. Let RW rebuild resources, using the actual client.
	auto& mode = (*inst->rwVideoModes)[modeIndex];
	const auto saved = mode;
	mode.width = inst->windowSizeClient.x;
	mode.height = inst->windowSizeClient.y;
	mode.flags &= ~1u;
	const auto result = injector::cstd<int(DWORD)>::call(0x7F8640, modeIndex);
	mode = saved;
	if (inst->RwEngineGetCurrentVideoMode() == modeIndex)
	{
		if (inst->modifiedVideoMode >= 0 && inst->modifiedVideoMode != (int)modeIndex)
			(*inst->rwVideoModes)[inst->modifiedVideoMode] = inst->videoModesBackup[inst->modifiedVideoMode];
		inst->modifiedVideoMode = (int)modeIndex;
		mode.flags &= ~1u;
	}
	return result;
}

void WindowedMode::WindowResize(POINT resolution)
{
	if (resolution.x < Resolution_Min.x || resolution.y < Resolution_Min.y) return;
	// Restore before assigning the requested size: restoring dispatches WM_SIZE.
	if (IsZoomed(window))
	{
		windowUpdating = true;
		ShowWindow(window, SW_RESTORE);
		windowUpdating = false;
	}
	if (windowMode == WindowMode::Fullscreen)
		windowMode = windowedStyle;

	windowSizeWindowed = resolution;
	WindowCalculateGeometry(true, true); // and resize the window
	SaveConfig();
}

void WindowedMode::WindowToggleFullscreen()
{
	if (IsIconic(window) || !HasFocus(window)) return;
	const auto monitor = GetMonitorRect({windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2});
	const POINT desktop = {monitor.right - monitor.left, monitor.bottom - monitor.top};
	const bool leaving = windowMode == Fullscreen;
	if (!leaving) windowedStyle = windowMode;
	if (IsZoomed(window))
	{
		windowUpdating = true;
		ShowWindow(window, SW_RESTORE);
		windowUpdating = false;
	}
	// A desktop-sized borderless window would otherwise look unchanged when
	// leaving fullscreen. Keep an unmistakably windowed restore size instead.
	const bool fallback = windowSizeWindowed.x == desktop.x && windowSizeWindowed.y == desktop.y;
	if (fallback) windowSizeWindowed = {640, 480};
	windowMode = leaving ? windowedStyle : Fullscreen;
	WindowCalculateGeometry(leaving && fallback, true);
	SaveConfig();
}

void WindowedMode::WindowToggleStyle()
{
	if (IsIconic(window) || !HasFocus(window)) return;
	if (IsZoomed(window))
	{
		windowUpdating = true;
		ShowWindow(window, SW_RESTORE);
		windowUpdating = false;
	}
	windowedStyle = windowedStyle == Windowed ? WindowedBorderless : Windowed;
	windowMode = windowedStyle;
	WindowCalculateGeometry(false, true);
	SaveConfig();
}

void WindowedMode::WindowUpdateTitle()
{
	if (windowMode == WindowMode::Windowed && HasFocus(window))
	{
		std::string aspectTxt;
		auto idx = FindAspectRatio(windowSizeClient);
		if (idx != -1)
		{
			aspectTxt = StringPrintf(" (%s)", AspectRatios[idx].name);
		}

		sprintf_s(windowTitle, "%s | %ux%u%s @ %u fps",
			rsGlobal->AppName,
			windowSizeClient.x,
			windowSizeClient.y,
			aspectTxt.c_str(),
			fpsCounter.get());
	}
	else
		strcpy_s(windowTitle, rsGlobal->AppName);
	
	if (window)
		SetWindowText(window, windowTitle);
}

LRESULT APIENTRY WindowedMode::WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) inst->window = wnd;
	// The original games send this malformed notification after a camera resize
	// fails while maximized. Neither our handler nor DefWindowProc can consume it.
	if (msg == WM_WINDOWPOSCHANGED && !lParam) return 0;
	// Reset can synchronously dispatch activation and position messages as well
	// as WM_SIZE. Never re-enter game/rendering code through those callbacks.
	if (inst->resetInProgress && msg != WM_STYLECHANGING)
		return DefWindowProc(wnd, msg, wParam, lParam);
	switch (msg)
	{
		// window focus/defocus
		case WM_ACTIVATE:
		{
			auto result = (LOWORD(wParam) == WA_INACTIVE) ?
				DefWindowProc(wnd, msg, wParam, lParam) : // don't pause game on defocus
				CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam);

			// handle automatic pause/resume
			if (inst->gameState == Playing_Game)
			{
				switch(LOWORD(wParam))
				{
					case WA_INACTIVE:
						if (inst->autoPause && !inst->IsMainMenuVisible())
						{
							inst->SwitchMainMenu(true);
							inst->autoPauseExecuted = true;
						}
						break;

					case WA_CLICKACTIVE: // mouse click
						if (!IsCursorInClientRect(wnd))
						{
							inst->autoPauseExecuted = false;
							break; // user clicked on the window caption or edge
						}
						[[fallthrough]];

					case WA_ACTIVE:
						if (inst->autoResume && 
							(!inst->autoPause || inst->autoPauseExecuted) && 
							inst->IsMainMenuVisible()) // TODO: check if not in some submenu
						{
							inst->autoPauseExecuted = false;
							inst->SwitchMainMenu(false);
						}
						break;
				}
			}

			inst->WindowUpdateTitle();
			inst->MouseUpdate(true);
			return result;
		}

		// don't pause game on defocus
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			return DefWindowProc(wnd, msg, wParam, lParam);
		
		// restore proper handling of ShowCursor
		case WM_SETCURSOR:
			return DefWindowProc(wnd, msg, wParam, lParam);

		// do not send keyboard events to the inactive window
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (!HasFocus(wnd))
				return DefWindowProc(wnd, msg, wParam, lParam); // bypass the game
			
			// handle Alt+Enter and Ctrl+Enter key combinations
			const bool alt = (lParam & (1L << 29)) != 0;
			const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (wParam == VK_RETURN && (alt || control || inst->enterShortcutDown))
			{
				if (!(lParam & (1L << 30)) && !inst->enterShortcutDown)
				{
					if (alt) inst->WindowToggleFullscreen();
					else if (control) inst->WindowToggleStyle();
				}
				inst->enterShortcutDown = true;
				return 0;
			}

			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
			if (wParam == VK_RETURN && inst->enterShortcutDown)
			{
				inst->enterShortcutDown = false;
				return 0;
			}
			break;

		// handle the window menu Alt+Key hotkey messages
		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_KEYMENU)
				return S_OK; // handled
			break;

		// do not send mouse events to the inactive window
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEACTIVATE:
		case WM_MOUSEHOVER:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		{
			auto hasFocus = HasFocus(wnd);
			auto inClient = IsCursorInClientRect(wnd);

			if (!hasFocus && inClient)
			{
				SetCursor(LoadCursor(NULL,IDC_ARROW));
			}

			if (!hasFocus || !inClient)
			{
				return DefWindowProc(wnd, msg, wParam, lParam); // bypass the game
			}
			break;
		}

		case WM_STYLECHANGING:
		{
			auto styles = (STYLESTRUCT*)lParam;
			
			if (wParam == GWL_STYLE)
			{
				auto mask = WS_MINIMIZE | WS_MAXIMIZE;
				styles->styleNew &= mask;
				styles->styleNew |= inst->WindowStyle();
			}
			else if (wParam == GWL_EXSTYLE)
				styles->styleNew = inst->WindowStyleEx();
				
			break;
		}

		// user dragging the window edge
		case WM_ENTERSIZEMOVE:
			inst->sizing = true;
			ClipCursor(NULL);
			return 0;

		case WM_GETMINMAXINFO:
		{
			auto limits = (MINMAXINFO*)lParam;
			limits->ptMinTrackSize = inst->SizeFromClient(Resolution_Min);
			return 0;
		}

		case WM_SIZING:
		{
			auto wndRect = (RECT*)lParam;
			auto size = inst->ClientFromSize({
				wndRect->right - wndRect->left,
				wndRect->bottom - wndRect->top
			});

			// minimal game resolution
			size.x = max(size.x, Resolution_Min.x);
			size.y = max(size.y, Resolution_Min.y);

			// snap to known aspect ratios
			auto idx = FindAspectRatio(size, 0.02f);
			if (idx != -1)
			{
				auto currAspect = float(size.x) / size.y;

				switch(wParam)
				{
					case WMSZ_LEFT:
					case WMSZ_RIGHT:
						size.x = LONG(size.y * AspectRatios[idx].ratio);
						break;

					case WMSZ_TOP:
					case WMSZ_BOTTOM:
						size.y = LONG(size.x / AspectRatios[idx].ratio);
						break;

					default: // sizing both X and Y
					{
						if (currAspect < AspectRatios[idx].ratio)
							size.x = LONG(size.y * AspectRatios[idx].ratio);
						else
							size.y = LONG(size.x / AspectRatios[idx].ratio);
					}
				}
			}

			// update window title immediately
			inst->windowSizeClient.x = size.x;
			inst->windowSizeClient.y = size.y;
			inst->WindowUpdateTitle();

			// apply modified window size
			size = inst->SizeFromClient(size);
			if (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT) wndRect->left = wndRect->right - size.x;
			if (wParam == WMSZ_RIGHT || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_BOTTOMRIGHT) wndRect->right = wndRect->left + size.x;
			if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT) wndRect->top = wndRect->bottom - size.y;
			if (wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT) wndRect->bottom = wndRect->top + size.y;

			return TRUE;
		}

		case WM_EXITSIZEMOVE:
			inst->sizing = false;
			inst->WindowCalculateGeometry(false, true);
			inst->SaveConfig();
			inst->MouseUpdate(true);
			return 0;

		// minimize, maximize, restore
		case WM_SIZE:
			if (!inst->windowUpdating && !inst->resetInProgress && !inst->sizing &&
				wParam != SIZE_MINIMIZED && wParam != SIZE_MAXHIDE && LOWORD(lParam) && HIWORD(lParam))
				CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam); // inform the game
			return DefWindowProc(wnd, msg, wParam, lParam); // call default as otherwise maximization will not work correctly on later Windows versions

		// position or size changed
		case WM_WINDOWPOSCHANGED:
		{
			if (inst->windowUpdating || inst->resetInProgress || IsIconic(wnd))
				return DefWindowProc(wnd, msg, wParam, lParam);

			bool updated = false;
			auto info = (WINDOWPOS*)lParam;

			// correct modern Windows styles invisible border
			RECT padding = inst->GetFrameSize(true);
			POINT pos = { info->x + padding.left, info->y + padding.top };
			if ((info->flags & SWP_NOMOVE) == 0)
			{
				if (pos.x != inst->windowPos.x || pos.y != inst->windowPos.y)
				{
					inst->windowPos = pos;
					updated = true;
				}
			}

			if ((info->flags & SWP_NOSIZE) == 0)
			{
				if (info->cx != inst->windowSize.x || info->cy != inst->windowSize.y)
				{
					inst->windowSize = { info->cx, info->cy };
					updated = true;
				}
			}

			if (updated)
			{
				RECT client;
				if (!GetClientRect(wnd, &client) || client.right <= 0 || client.bottom <= 0)
					return DefWindowProc(wnd, msg, wParam, lParam);
				inst->windowSizeClient = { client.right, client.bottom };
				if (inst->windowMode != WindowMode::Fullscreen && !IsZoomed(wnd))
				{
					inst->windowPosWindowed = inst->windowPos;
					inst->windowSizeWindowed = inst->windowSizeClient;
				}
				inst->WindowCalculateGeometry();
				inst->WindowUpdateTitle();
				if (!inst->sizing) inst->SaveConfig();
			}

			// DefWindowProc generates WM_SIZE/WM_MOVE. The game's handler must not
			// run a second time or resize the camera while a Reset is in progress.
			return DefWindowProc(wnd, msg, wParam, lParam);
		}
	}

	return CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam);
}

POINT WindowedMode::SizeFromClient(POINT clientSize) const
{
	auto frame = GetFrameSize();
	clientSize.x += frame.left + frame.right;
	clientSize.y += frame.top + frame.bottom;
	return clientSize;
}

POINT WindowedMode::ClientFromSize(POINT windowSize) const
{
	auto frame = GetFrameSize();
	windowSize.x -= frame.left + frame.right;
	windowSize.y -= frame.top + frame.bottom;
	return windowSize;
}

RECT WindowedMode::GetFrameSize(bool padOnly) const
{
	RECT frame = { 0 };

	if (padOnly)
	{
		RECT base, extended;
		if (GetWindowRect(window, &base) &&
			SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &extended, sizeof(RECT))))
		{
			frame.left = extended.left - base.left;
			frame.top = extended.top - base.top;
			frame.right = base.right - extended.right;
			frame.bottom = base.bottom - extended.bottom;
		}
	}
	else
	{
		using GetDpi = UINT(WINAPI*)(HWND);
		using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
		static const auto user32 = GetModuleHandleA("user32.dll");
		static const auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForWindow"));
		static const auto adjustForDpi = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
		if (!window || !getDpi || !adjustForDpi ||
			!adjustForDpi(&frame, WindowStyle(), false, WindowStyleEx(), getDpi(window)))
			AdjustWindowRectEx(&frame, WindowStyle(), false, WindowStyleEx());
		frame.left *= -1; // offset to thickness
		frame.top *= -1; // offset to thickness
	}

	return frame;
}

bool WindowedMode::IsD3D9() const
{
	return gameTitle == GameTitle::GTA_SA;
}

HRESULT WindowedMode::D3dPresentHook(IDirect3DDevice8* self, const RECT* srcRect, const RECT* dstRect, HWND wnd, const RGNDATA* region)
{
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

	return inst->d3dPresentOri(self, srcRect, dstRect, wnd, region);
}

HRESULT WindowedMode::D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters)
{
	if (!parameters || inst->resetInProgress) return D3DERR_INVALIDCALL;
	inst->resetInProgress = true;
	inst->postEffectUpdatePending = false;
	// Preserve the caller's structure, including AA/depth settings and rollback
	// sizes. D3D8 and D3D9 have different layouts after MultiSampleType.
	// Zero dimensions mean the current client size, not a menu request.
	auto reset = [self](auto& caller) {
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
		const auto result = inst->d3dResetOri(self, reinterpret_cast<D3DPRESENT_PARAMETERS*>(&driverParameters));
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

bool WindowedMode::IsMainMenuVisible() const
{
	switch(gameTitle)
	{
		case GTA_3:
		{
			auto mgr = (CMenuManager3*)frontEndMenuManager;
			return mgr->m_bMenuActive;
		}

		case GTA_VC:
		{
			auto mgr = (CMenuManagerVC*)frontEndMenuManager;
			return mgr->m_bMenuActive;
		}
		
		case GTA_SA:
		{
			auto mgr = (CMenuManagerSA*)frontEndMenuManager;
			return mgr->m_bMenuActive;
		}

		default:
			return false;
	}
}

void WindowedMode::SwitchMainMenu(bool show)
{
	switch(gameTitle)
	{
		case GTA_3:
			if (show)
				injector::cstd<void()>::call(0x488770); // CMenuManager::RequestFrontEndStartUp()
			else
				injector::cstd<void()>::call(0x488750); // CMenuManager::RequestFrontEndShutDown()
			break;

		case GTA_VC:
		{
			auto mgr = (CMenuManagerVC*)frontEndMenuManager;
			if (show == mgr->m_bMenuActive) break; // already done
			
			mgr->m_bStartUpFrontEndRequested = show;
			mgr->m_bShutDownFrontEndRequested = !show;
			break;
		}
		
		case GTA_SA:
		{
			auto mgr = (CMenuManagerSA*)frontEndMenuManager;
			if (show == mgr->m_bMenuActive) break; // already done

			mgr->m_bActivateMenuNextFrame = show;
			mgr->m_bDontDrawFrontEnd = !show;
			break;
		}
	}
}

void WindowedMode::MouseUpdate(bool force)
{
	auto hasFocus = HasFocus(window);

	POINT pos;
	GetCursorPos(&pos);

	RECT rect;
	GetClientRect(window, &rect);
	ClientToScreen(window, (LPPOINT)&rect.left);
	ClientToScreen(window, (LPPOINT)&rect.right);

	// cursor visibility
	bool inGame = hasFocus && PtInRect(&rect, pos);
	SetCursorVisible(!inGame);

	// keep cursor inside the window
	if (hasFocus || force)
	{
		if (!hasFocus || IsMainMenuVisible())
			ClipCursor(NULL);
		else
			ClipCursor(&rect);
	}
}

void WindowedMode::UpdatePostEffect()
{
	switch(gameTitle)
	{
		case GameTitle::GTA_3:
			if (auto cam = *(RwCamera**)0x72676C; cam && cam->frameBuffer)
				injector::cstd<void(RwCamera*)>::call(0x50AE40, cam); // CMBlurMotion::BlurOpen(RwCamera*)
			break;
			
		case GameTitle::GTA_VC:
			if (auto cam = *(RwCamera**)0x8100BC; cam && cam->frameBuffer)
				injector::cstd<void(RwCamera*)>::call(0x55CE20, cam); // CMBlurMotion::BlurOpen(RwCamera*)
			break;
			
		case GameTitle::GTA_SA:
		{
			auto cam = *(RwCamera**)0xC1703C; // Scene.m_pRwCamera
			if (cam && cam->frameBuffer)
				injector::cstd<void()>::call(0x7043D0); // CPostEffects::SetupBackBufferVertex()
			break;
		}
	}

	UpdateWidescreenFix();
}

void WindowedMode::UpdateWidescreenFix()
{
	static bool initialized = false;
	static HMODULE widescreenFix = NULL;
	static FARPROC updateFunc = NULL;

	if (!initialized)
	{
		switch(gameTitle)
		{
			case GameTitle::GTA_3:
				widescreenFix = GetModuleHandle("GTA3.WidescreenFix.asi");
				break;
			
			case GameTitle::GTA_VC:
				widescreenFix = GetModuleHandle("GTAVC.WidescreenFix.asi");
				break;
			
			case GameTitle::GTA_SA:
				widescreenFix = GetModuleHandle("GTASA.WidescreenFix.asi");
				break;
		}

		if (widescreenFix)
			updateFunc = GetProcAddress(widescreenFix, "UpdateVars");

		initialized = true;
	}

	if (updateFunc)
		injector::stdcall<void()>::call(updateFunc);
}


#pragma once
#include "misc.h"
#include <unordered_map>

class WindowedMode
{
public:
	static constexpr POINT Resolution_Default = { 640, 448 }; // default GTA's PS2 resolution
	static constexpr POINT Resolution_Min = { Resolution_Default.x / 4, Resolution_Default.y / 4 };

	enum GameTitle : BYTE { GTA_3, GTA_VC, GTA_SA };

	static void InitGta3();
	static void InitGtaVC();
	static void InitGtaSA();

	// game internals
	const GameTitle gameTitle;
	GameState& gameState;
	union{ RsGlobalType* rsGlobal; RsGlobalTypeSA* rsGlobalSA; }; // different in SA
	WNDPROC oriWindowProc;
	IDirect3DDevice8* &d3dDevice;
	union{ D3DPRESENT_PARAMETERS* d3dPresentParams8; D3DPRESENT_PARAMETERS_D3D9* d3dPresentParams9; }; // DirectX 8 or 9
	DisplayMode** rwVideoModes; // array
	DWORD (*RwEngineGetNumVideoModes)();
	DWORD (*RwEngineGetCurrentVideoMode)();
	uintptr_t frontEndMenuManager;
	std::vector<DisplayMode> videoModesBackup;
	DisplayMode* videoModesSource = nullptr;
	int modifiedVideoMode = -1;

	// constructor taking addresses for specific game version
	WindowedMode(
		GameTitle gameTitle,
		uintptr_t gameState,
		uintptr_t rsGlobal,
		uintptr_t d3dDevice,
		uintptr_t d3dPresentParams,
		uintptr_t rwVideoModes,
		uintptr_t RwEngineGetNumVideoModes,
		uintptr_t RwEngineGetCurrentVideoMode,
		uintptr_t frontEndMenuManager
	);

	static HWND __stdcall InitWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	void InitD3dDevice(); // instal hooks

	// config file
	CIniReader config;
	void InitConfig(); // create default ini file
	bool LoadConfig(); // returns true if maximized
	void SaveConfig();

	// game window
	struct AspectRatioInfo { const char* name; float ratio; };
	static const AspectRatioInfo AspectRatios[];
	static int FindAspectRatio(POINT resolution, float treshold = 0.007f);

	enum WindowMode : BYTE
	{
		Windowed = 1, WindowedBorderless, Fullscreen,
		Min = Windowed,
		Max = Fullscreen
	};
	
	HWND window = 0;
	bool windowUpdating = false; // update in progress
	bool resetInProgress = false;
	bool sizing = false;
	bool postEffectUpdatePending = false;
	WindowMode windowMode = WindowMode::Windowed;
	WindowMode windowedStyle = WindowMode::Windowed;
	bool enterShortcutDown = false;
	HICON windowIcon = NULL;
	char windowClassName[64];
	char windowTitle[64];
	// current window geometry
	POINT windowPos = {};
	POINT windowSize = {};
	POINT windowSizeClient = {};
	// last properties of windowed mode
	POINT windowPosWindowed = { -1, -1 };
	POINT windowSizeWindowed = Resolution_Default;

	void WindowCalculateGeometry(bool center = false, bool resizeWindow = false);
	void WindowResize(POINT resolution);
	void ChangeResolution(DWORD modeIndex);
	void BackupVideoModes();
	void InitPresentationParameters();
	static void __cdecl ChangeResolutionSA(DWORD modeIndex);
	static int __cdecl ChangeVideoModeSA(DWORD modeIndex);
	void WindowToggleFullscreen();
	void WindowToggleStyle();
	DWORD WindowStyle() const;
	DWORD WindowStyleEx() const;
	void WindowUpdateTitle();
	static LRESULT APIENTRY WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	POINT SizeFromClient(POINT clientSize) const;
	POINT ClientFromSize(POINT windowSize) const;
	RECT GetFrameSize(bool paddOnly = false) const; // size of window frame and extra padding/shadow introduced in later versions of Windows
	
	// DirectX 3D related stuff
	bool IsD3D9() const;

	HRESULT static __stdcall D3dPresentHook(IDirect3DDevice8* self, const RECT* srcRect, const RECT* dstRect, HWND wnd, const RGNDATA* region);
	decltype(D3dPresentHook)* d3dPresentOri = nullptr;

	HRESULT static __stdcall D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters);
	decltype(D3dResetHook)* d3dResetOri = nullptr;

	// other
	FpsCounter fpsCounter;
	bool autoPause = true;
	bool autoResume = true;
	bool autoPauseExecuted = false;

	bool IsMainMenuVisible() const;
	void SwitchMainMenu(bool show);
	
	void MouseUpdate(bool force = false);
	void UpdatePostEffect();
	void UpdateWidescreenFix();
};

static WindowedMode* inst; // global instance


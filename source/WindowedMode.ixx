module;
#include "stdafx.h"
#include "Bindings.h"

export module WindowedMode;

export class WindowedMode
{
public:
    static constexpr POINT Resolution_Default = { 640, 448 }; // default GTA's PS2 resolution
    static constexpr POINT Resolution_Min = { Resolution_Default.x / 4, Resolution_Default.y / 4 };

    using GameTitle = ::GameTitle;
    static constexpr auto GTA_3 = GameTitle::GTA3;
    static constexpr auto GTA_VC = GameTitle::GTAVC;
    static constexpr auto GTA_SA = GameTitle::GTASA;

    // game internals
    const GameBindings bindings;
    const GameTitle gameTitle;
    GameState& gameState;
    union{ RsGlobalType* rsGlobal; RsGlobalTypeSA* rsGlobalSA; }; // different in SA
    WNDPROC oriWindowProc = nullptr;
    inline static decltype(&CreateWindowExA) createWindow = &CreateWindowExA;
    IDirect3DDevice8* d3dDevice = nullptr;
    union{ D3DPRESENT_PARAMETERS* d3dPresentParams8; D3DPRESENT_PARAMETERS_D3D9* d3dPresentParams9; }; // DirectX 8 or 9
    DisplayMode** rwVideoModes; // array
    DWORD (*RwEngineGetNumVideoModes)();
    DWORD (*RwEngineGetCurrentVideoMode)();

    std::vector<DisplayMode> videoModesBackup;
    DisplayMode* videoModesSource = nullptr;
    int modifiedVideoMode = -1;

    explicit WindowedMode(const GameBindings& bindings);

    static HWND __stdcall InitWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    void InitD3dDevice(); // install hooks

    // config file
    CIniReader config;
    void InitConfig(); // create default ini file
    bool LoadConfig(); // returns true if maximized
    void SaveConfig();

    // game window
    struct AspectRatioInfo { const char* name; float ratio; };
    static const AspectRatioInfo AspectRatios[];
    static int FindAspectRatio(POINT resolution, float threshold = 0.007f);

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

    HRESULT static __stdcall D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters);

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

export inline WindowedMode* inst = nullptr;


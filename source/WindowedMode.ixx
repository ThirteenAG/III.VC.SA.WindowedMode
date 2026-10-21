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
    void InstallRenderHooks();
    void BeforeReset();
    void AfterReset(HRESULT result);
    void BeforePresent();
    std::array<uint8_t, sizeof(D3DPRESENT_PARAMETERS_D3D9)> resetSaved{}, resetEffective{};

    // settings
    void LoadConfig();
    void SaveConfig();
    void ApplySettings();

    // game window
    struct AspectRatioInfo { const char* name; float ratio; };
    static const AspectRatioInfo AspectRatios[];
    static int FindAspectRatio(POINT resolution, float threshold = 0.007f);

    // The values of MAIN.WindowMode; Fullscreen leaves the game as it is
    enum WindowMode : BYTE
    {
        Fullscreen = 0,
        Windowed = 1,
        Borderless = 2,
    };

    static constexpr UINT WM_APPLY_SETTINGS = WM_APP + 0x5157;

    HWND window = 0;
    bool windowUpdating = false; // update in progress
    bool resetInProgress = false;
    bool sizing = false;
    bool postEffectUpdatePending = false;
    WindowMode windowMode = WindowMode::Windowed;
    bool enterShortcutDown = false;

    char windowTitle[64];
    // current window geometry
    POINT windowPos = {};
    POINT windowSize = {};
    POINT windowSizeClient = {};
    // windowed mode: client size (0 = desktop size) and a point on the monitor it opens on
    POINT windowSizeWindowed = {};
    POINT windowMonitorPoint = { -1, -1 };
    bool windowMaximized = false;

    HMONITOR CurrentMonitor() const;
    POINT DesiredClientSize(const RECT& monitor) const;
    void WindowCalculateGeometry(bool resizeWindow = false, HMONITOR monitor = nullptr);
    void WindowResize(POINT resolution);
    void ChangeResolution(DWORD modeIndex);
    void BackupVideoModes();
    void InitPresentationParameters();
    static void __cdecl ChangeResolutionSA(DWORD modeIndex);
    static int __cdecl ChangeVideoModeSA(DWORD modeIndex);
    void WindowSetMode(WindowMode mode);
    void WindowToggleFullscreen();
    DWORD WindowStyle() const;
    DWORD WindowStyleEx() const;
    void WindowUpdateTitle();
    static LRESULT APIENTRY WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    POINT SizeFromClient(POINT clientSize) const;
    POINT ClientFromSize(POINT windowSize) const;
    RECT GetFrameSize(bool paddOnly = false) const; // size of window frame and extra padding/shadow introduced in later versions of Windows

    // DirectX 3D related stuff
    bool IsD3D9() const;

    // other
    FpsCounter fpsCounter;
    bool autoPauseExecuted = false;
    bool cursorClipped = false;

    bool IsMainMenuVisible() const;
    void SwitchMainMenu(bool show);

    void MouseUpdate(bool force = false);
    void UpdatePostEffect();
};
export inline WindowedMode* inst = nullptr;


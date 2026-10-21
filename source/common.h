#pragma once
#include <stdint.h>
#include "d3d8/d3d8.h"
#include "d3d8/dinput.h"
#include "injector/injector.hpp"



class FpsCounter
{
protected:
    unsigned int fps;
    unsigned int count;
    DWORD prevTime;

public:
    FpsCounter() : fps(30), count(0), prevTime(timeGetTime() / 1000)
    {
    }

    bool update()
    {
        auto currTime = timeGetTime() / 1000;
        if (prevTime != currTime)
        {
            fps = count;
            prevTime = currTime;
            count = 1; // this frame is in next period already
        }
        else
            count++;

        static DWORD prevFps = -1;
        auto updated = fps != prevFps;
        prevFps = fps;

        return updated;
    }

    unsigned int get() const
    {
        return fps;
    }
};

typedef struct _D3DPRESENT_PARAMETERS_D3D9_
{
    UINT BackBufferWidth;
    UINT BackBufferHeight;
    D3DFORMAT BackBufferFormat;
    UINT BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    D3DSWAPEFFECT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    D3DFORMAT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS_D3D9;

struct RsPlatformSpecific
{
    HWND window;
    HINSTANCE instance;
    DWORD fullScreen;
    float lastMousePos[2];
    DWORD lastRenderTime;
    LPDIRECTINPUT8 diInterface;
    LPDIRECTINPUTDEVICE8 diMouse;
    LPDIRECTINPUTDEVICE8 diDevice1;
    LPDIRECTINPUTDEVICE8 diDevice2;
};

struct RsGlobalType
{
    char* AppName;
    int MaximumWidth;
    int MaximumHeight;
    int screenWidth;
    int screenHeight;
    int frameLimit;
    int quit;
    RsPlatformSpecific* ps;
};

struct RsGlobalTypeSA
{
    char* AppName;
    int MaximumWidth;
    int MaximumHeight;
    int frameLimit;
    int quit;
    RsPlatformSpecific* ps;
};

typedef float RwReal;
struct RwV2d
{
    RwReal x;
    RwReal y;
};

struct RwV3d
{
    RwReal x;
    RwReal y;
    RwReal z;
};

struct RwPlane
{
    RwV3d normal;
    RwReal distance;
};

struct RwFrustumPlane
{
    RwPlane plane;
    uint8_t closestX;
    uint8_t closestY;
    uint8_t closestZ;
    uint8_t pad;
};

struct RwBBox
{
    RwV3d sup;
    RwV3d inf;
};

struct RwRaster
{
    RwRaster* pParent;
    uint8_t* pPixels;
    uint8_t* pPalette;
    int32_t nWidth;
    int32_t nHeight;
    int32_t nDepth;
    int32_t nStride;
    int16_t nOffsetX;
    int16_t nOffsetY;
    uint8_t cType;
    uint8_t cFlags;
    uint8_t cPrivateFlags;
    uint8_t cFormat;
    uint8_t* pOriginalPixels;
    int32_t nOriginalWidth;
    int32_t nOriginalHeight;
    int32_t nOriginalStride;
};

struct RwCamera
{
    char RwObjectHasFrame[20];
    uint32_t RwCameraProjection;
    uint32_t RwCameraBeginUpdateFunc;
    uint32_t RwCameraEndUpdateFunc;
    char RwMatrix[64];
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
    RwReal zScale, zShift;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};

enum GameState : DWORD
{
    Startup,
    Init_Logo_Mpeg,
    Logo_Mpeg,
    Init_Intro_Mpeg,
    Intro_Mpeg,
    Init_Once,
    Init_Frontend,
    Frontend,
    Init_Playing_Game,
    Playing_Game,
};

struct DisplayMode
{
    //D3DDISPLAYMODE mode; expanded below
    UINT width;
    UINT height;
    UINT refreshRate;
    D3DFORMAT format;

    DWORD flags;
};

static inline RECT GetMonitorRect(POINT pos)
{
    auto monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = { sizeof(MONITORINFO) };

    if (!GetMonitorInfo(monitor, &info))
    {
        monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
        GetMonitorInfo(monitor, &info);
    }

    return info.rcMonitor;
}

static inline bool HasFocus(HWND wnd)
{
    return GetForegroundWindow() == wnd;
}

static inline bool IsCursorInClientRect(HWND wnd)
{
    POINT pos;
    GetCursorPos(&pos);

    RECT rect;
    GetClientRect(wnd, &rect);
    ClientToScreen(wnd, (LPPOINT)&rect.left); // left& top
    ClientToScreen(wnd, (LPPOINT)&rect.right); // right & bottom

    return PtInRect(&rect, pos);
}

static inline std::string StringPrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    auto len = std::vsnprintf(nullptr, 0, format, args) + 1;
    va_end(args);

    std::string result(len, '\0');

    va_start(args, format);
    std::vsnprintf(result.data(), result.length(), format, args);
    va_end(args);

    result.resize(len - 1);
    return result;
}

static inline void SetCursorVisible(bool show)
{
    int targetState = show ? 0 : -1;
    int currState = ShowCursor(show);
    int tries = 128; // infinite loop prevention
    while (currState != targetState && tries)
    {
        currState = ShowCursor(currState < targetState);
        tries--;
    }
}

template <class ... Args>
static void ShowError(const char* format, Args ... args)
{
    auto msg = StringPrintf(format, args...);

    auto wnd = GetActiveWindow();
    if (wnd)
    {
        PostMessage(wnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ShowWindow(wnd, SW_MINIMIZE);
    }

    SetCursorVisible(true);
    MessageBoxA(wnd, msg.c_str(), rsc_ProductName, MB_SYSTEMMODAL | MB_ICONERROR);
}


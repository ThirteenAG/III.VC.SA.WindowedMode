module;
#include "stdafx.h"
#include "Bindings.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

module WindowedMode;

WindowedMode::WindowedMode(const GameBindings& resolved) :
    bindings(resolved), gameTitle(resolved.title), gameState(*resolved.state),
    rsGlobal(resolved.globals), d3dPresentParams8(nullptr), rwVideoModes(resolved.modes),
    RwEngineGetNumVideoModes(resolved.getNumVideoModes),
    RwEngineGetCurrentVideoMode(resolved.getCurrentVideoMode)
{}

const WindowedMode::AspectRatioInfo WindowedMode::AspectRatios[] = {
    { "1:1", 1.0f / 1.0f },
    { "2:1", 2.0f / 1.0f },
    { "4:3", 4.0f / 3.0f },
    { "5:4", 5.0f / 4.0f },
    { "10:7", 10.0f / 7.0f }, // GTA default aspect
    { "16:9", 16.0f / 9.0f },
    { "16:10", 16.0f / 10.0f }
};

HWND __stdcall WindowedMode::InitWindow(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style,
    int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE module, LPVOID param)
{
    inst->InitConfig();
    const bool maximize = inst->LoadConfig();
    const bool center = inst->windowPos.x == -1 || inst->windowPos.y == -1;
    inst->WindowCalculateGeometry(center);
    inst->window = createWindow(inst->WindowStyleEx(), className, title, inst->WindowStyle(),
        inst->windowPos.x, inst->windowPos.y, inst->windowSize.x, inst->windowSize.y,
        parent, menu, module, param);
    if (!inst->window) return nullptr;

    // Use a Unicode subclass consistently. USER32 converts messages when
    // CallWindowProcW forwards to the game's original ANSI procedure.
    inst->oriWindowProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(inst->window,
        GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowProc)));
    if (maximize && inst->windowMode != Fullscreen)
        ShowWindow(inst->window, SW_MAXIMIZE);
    else
        inst->WindowCalculateGeometry(center, true);
    inst->WindowUpdateTitle();
    UpdateWindow(inst->window);
    return inst->window;
}

int WindowedMode::FindAspectRatio(POINT resolution, float threshold)
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

    return dist <= threshold ? idx : -1;
}

DWORD WindowedMode::WindowStyle() const
{
    return WS_VISIBLE | WS_CLIPSIBLINGS | ((windowMode == WindowMode::Windowed) ?
        WS_OVERLAPPEDWINDOW :
        WS_POPUP);
}

DWORD WindowedMode::WindowStyleEx() const
{
    return 0;
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
    if (gameTitle == GameTitle::GTASA)
    {
        if (rsGlobalSA->ps) { rsGlobalSA->ps->fullScreen = false; rsGlobalSA->ps->window = window; }
        rsGlobalSA->MaximumWidth = windowSizeClient.x;
        rsGlobalSA->MaximumHeight = windowSizeClient.y;
    }
    else
    {
        if (rsGlobal->ps) { rsGlobal->ps->fullScreen = false; rsGlobal->ps->window = window; }
        rsGlobal->screenWidth = rsGlobal->MaximumWidth = windowSizeClient.x;
        rsGlobal->screenHeight = rsGlobal->MaximumHeight = windowSizeClient.y;
    }

    // Do not edit Present here: it belongs to RenderWare's reset/recovery path.
    // In particular, WM_SIZE can be dispatched synchronously from inside Reset.
    windowUpdating = false;
    if (resizeWindow && window && d3dDevice && !resetInProgress)
        CallWindowProcW(oriWindowProc, window, WM_SIZE, SIZE_RESTORED,
            MAKELPARAM(windowSizeClient.x, windowSizeClient.y));
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
        SetWindowTextA(window, windowTitle);
}

LRESULT APIENTRY WindowedMode::WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE) inst->window = wnd;
    // Caption messages belong to this Unicode subclass. Do not pass their text
    // buffers through game/mod procedures that may assume a different encoding.
    if (msg == WM_SETTEXT || msg == WM_GETTEXT || msg == WM_GETTEXTLENGTH)
        return DefWindowProcW(wnd, msg, wParam, lParam);
    // The original games send this malformed notification after a camera resize
    // fails while maximized. Neither our handler nor DefWindowProc can consume it.
    if (msg == WM_WINDOWPOSCHANGED && !lParam) return 0;
    // Reset can synchronously dispatch activation and position messages as well
    // as WM_SIZE. Never re-enter game/rendering code through those callbacks.
    if (inst->resetInProgress && msg != WM_STYLECHANGING)
        return DefWindowProcW(wnd, msg, wParam, lParam);
    switch (msg)
    {
        // window focus/defocus
        case WM_ACTIVATE:
        {
            auto result = (LOWORD(wParam) == WA_INACTIVE) ?
                DefWindowProcW(wnd, msg, wParam, lParam) : // don't pause game on defocus
                CallWindowProcW(inst->oriWindowProc, wnd, msg, wParam, lParam);

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
            inst->enterShortcutDown = false;
            return DefWindowProcW(wnd, msg, wParam, lParam);
        
        // restore proper handling of ShowCursor
        case WM_SETCURSOR:
            return DefWindowProcW(wnd, msg, wParam, lParam);

        // do not send keyboard events to the inactive window
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (!HasFocus(wnd))
                return DefWindowProcW(wnd, msg, wParam, lParam); // bypass the game
            
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
                return DefWindowProcW(wnd, msg, wParam, lParam); // bypass the game
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
                CallWindowProcW(inst->oriWindowProc, wnd, msg, wParam, lParam); // inform the game
            return DefWindowProcW(wnd, msg, wParam, lParam); // call default as otherwise maximization will not work correctly on later Windows versions

        // position or size changed
        case WM_WINDOWPOSCHANGED:
        {
            if (inst->windowUpdating || inst->resetInProgress || IsIconic(wnd))
                return DefWindowProcW(wnd, msg, wParam, lParam);

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
                    return DefWindowProcW(wnd, msg, wParam, lParam);
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
            return DefWindowProcW(wnd, msg, wParam, lParam);
        }
    }

    return CallWindowProcW(inst->oriWindowProc, wnd, msg, wParam, lParam);
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

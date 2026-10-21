module;
#include "stdafx.h"
#include "Bindings.h"
#include "Settings.h"
#include <WindowStyle.hpp>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

module WindowedMode;

namespace
{
    // The DPI of a monitor (the system's before Windows 8.1)
    UINT MonitorDpi(HMONITOR monitor)
    {
        using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
        static auto getDpiForMonitor = []() -> GetDpiForMonitorFn
        {
            auto shcore = LoadLibraryExW(L"shcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return shcore ? reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor")) : nullptr;
        }();
        UINT x = 0, y = 0;
        if (getDpiForMonitor && SUCCEEDED(getDpiForMonitor(monitor, 0, &x, &y)) && x)
            return x;
        return ::WindowStyle::DpiOf(nullptr);
    }
}

WindowedMode::WindowedMode(const GameBindings& resolved) :
    bindings(resolved), gameTitle(resolved.title), gameState(*resolved.state),
    rsGlobal(resolved.globals), d3dPresentParams8(resolved.presentParameters), rwVideoModes(resolved.modes),
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

HWND __stdcall WindowedMode::InitWindow(DWORD, LPCSTR className, LPCSTR title, DWORD,
    int, int, int, int, HWND parent, HMENU menu, HINSTANCE module, LPVOID param)
{
    try
    {
        inst->InstallRenderHooks();
    }
    catch (const std::exception& error)
    {
        ShowError("%s", error.what());
        return nullptr;
    }
    inst->LoadConfig();

    // The monitor the window was on last time, the primary one at first
    auto monitor = inst->windowMonitorPoint.x == -1 && inst->windowMonitorPoint.y == -1 ?
        MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint(inst->windowMonitorPoint, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{ sizeof(info) };
    GetMonitorInfo(monitor, &info);
    auto client = inst->DesiredClientSize(info.rcMonitor);
    auto frame = ::WindowStyle::FrameOf(inst->WindowStyle(), inst->WindowStyleEx(), MonitorDpi(monitor));
    auto rect = ::WindowStyle::WindowRect(info.rcMonitor, client.x, client.y, frame);
    inst->windowPos = { rect.left, rect.top };
    inst->windowSize = { rect.right - rect.left, rect.bottom - rect.top };
    inst->windowSizeClient = client;

    inst->window = createWindow(inst->WindowStyleEx(), className, title, inst->WindowStyle(),
        inst->windowPos.x, inst->windowPos.y, inst->windowSize.x, inst->windowSize.y,
        parent, menu, module, param);
    if (!inst->window)
        return nullptr;

    // Keep the game's ANSI window procedure chain. Switching it to Unicode
    // gives ANSI subclasses a conversion token that some mods call directly.
    inst->oriWindowProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(inst->window,
        GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowProc)));
    inst->WindowCalculateGeometry(true, monitor);
    if (inst->windowMaximized && inst->windowMode == Windowed)
        ShowWindow(inst->window, SW_MAXIMIZE);
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

HMONITOR WindowedMode::CurrentMonitor() const
{
    return MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
}

// The client size the mode wants on a monitor: the monitor's size in Borderless, the chosen size in Windowed
POINT WindowedMode::DesiredClientSize(const RECT& monitor) const
{
    POINT desktop = { monitor.right - monitor.left, monitor.bottom - monitor.top };
    if (windowMode == WindowMode::Borderless || windowSizeWindowed.x <= 0 || windowSizeWindowed.y <= 0)
        return desktop;
    return windowSizeWindowed;
}

void WindowedMode::WindowCalculateGeometry(bool resizeWindow, HMONITOR monitor)
{
    if (windowUpdating)
        return;
    windowUpdating = true;

    // apply to the window: centered on its monitor; at the monitor's size the client area covers it and a frame lies
    // outside the screen
    if (resizeWindow && window && !IsZoomed(window))
    {
        if (!monitor)
            monitor = CurrentMonitor();
        SetWindowLong(window, GWL_STYLE, WindowStyle());
        SetWindowLong(window, GWL_EXSTYLE, WindowStyleEx());
        SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW); // update the frame
        // Placed here rather than with a window subclass: wrappers that chain the window procedure break when it
        // becomes a Unicode one
        auto monitorRect = ::WindowStyle::MonitorRect(window, monitor);
        auto client = DesiredClientSize(monitorRect);
        auto frame = ::WindowStyle::FrameOf(window);
        auto rect = ::WindowStyle::WindowRect(monitorRect, client.x, client.y, frame);
        windowPos = { rect.left, rect.top };
        windowSize = { rect.right - rect.left, rect.bottom - rect.top };
        MoveWindow(window, rect.left, rect.top, windowSize.x, windowSize.y, TRUE);
        ::WindowStyle::SquareCorners(window);
        bool desktopSize = ::WindowStyle::CoversMonitor(monitorRect, client.x, client.y);
        ::WindowStyle::detail::BorderColor(window, desktopSize && frame.Height() > 0);
        ::WindowStyle::detail::MarkFullscreen(window, desktopSize);
        WindowUpdateTitle();
    }
    else if (window)
        ::WindowStyle::SquareCorners(window);

    // Windows is authoritative (DPI, maximization and frame metrics can change).
    RECT client;
    if (window && !IsIconic(window) && GetClientRect(window, &client) &&
        client.right > 0 && client.bottom > 0)
    {
        windowSizeClient = { client.right, client.bottom };
        // the user resized the window
        if (!resizeWindow && !IsZoomed(window) && windowMode == WindowMode::Windowed)
            windowSizeWindowed = windowSizeClient;
    }

    // apply resolution to game internals
    if (gameTitle == GameTitle::GTASA)
    {
        if (rsGlobalSA->ps)
        {
            rsGlobalSA->ps->fullScreen = false;
            rsGlobalSA->ps->window = window;
        }
        rsGlobalSA->MaximumWidth = windowSizeClient.x;
        rsGlobalSA->MaximumHeight = windowSizeClient.y;
    }
    else
    {
        if (rsGlobal->ps)
        {
            rsGlobal->ps->fullScreen = false;
            rsGlobal->ps->window = window;
        }
        rsGlobal->screenWidth = rsGlobal->MaximumWidth = windowSizeClient.x;
        rsGlobal->screenHeight = rsGlobal->MaximumHeight = windowSizeClient.y;
    }

    // Do not edit Present here: it belongs to RenderWare's reset/recovery path.
    // In particular, WM_SIZE can be dispatched synchronously from inside Reset.
    windowUpdating = false;
    if (resizeWindow && window && d3dDevice && !resetInProgress)
        CallWindowProcA(oriWindowProc, window, WM_SIZE, SIZE_RESTORED,
            MAKELPARAM(windowSizeClient.x, windowSizeClient.y));
}

// A resolution chosen in the game's menu: a window of that size
void WindowedMode::WindowResize(POINT resolution)
{
    if (resolution.x < Resolution_Min.x || resolution.y < Resolution_Min.y)
        return;
    // Restore before assigning the requested size: restoring dispatches WM_SIZE.
    if (IsZoomed(window))
    {
        windowUpdating = true;
        ShowWindow(window, SW_RESTORE);
        windowUpdating = false;
    }
    windowMode = WindowMode::Windowed;
    windowSizeWindowed = resolution;
    WindowCalculateGeometry(true);
    SaveConfig();
}

void WindowedMode::WindowSetMode(WindowMode mode)
{
    if (mode == windowMode || mode == WindowMode::Fullscreen)
        return;
    auto monitor = CurrentMonitor();
    if (IsZoomed(window))
    {
        windowUpdating = true;
        ShowWindow(window, SW_RESTORE);
        windowUpdating = false;
    }
    windowMode = mode;
    WindowCalculateGeometry(true, monitor);
    SaveConfig();
}

// Alt+Enter: Windowed <-> Borderless
void WindowedMode::WindowToggleFullscreen()
{
    if (IsIconic(window) || !HasFocus(window))
        return;
    if (windowMode == WindowMode::Borderless)
    {
        // A window as big as the desktop would look unchanged: leave with a smaller one
        auto monitor = ::WindowStyle::MonitorRect(window);
        if (windowSizeWindowed.x <= 0 || ::WindowStyle::CoversMonitor(monitor, windowSizeWindowed.x, windowSizeWindowed.y))
            windowSizeWindowed = { 640, 480 };
        WindowSetMode(WindowMode::Windowed);
    }
    else
        WindowSetMode(WindowMode::Borderless);
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
    // Handle caption buffers in the same ANSI encoding as CreateWindowExA
    // and SetWindowTextA.
    if (msg == WM_SETTEXT || msg == WM_GETTEXT || msg == WM_GETTEXTLENGTH)
        return DefWindowProcA(wnd, msg, wParam, lParam);
    // The original games send this malformed notification after a camera resize
    // fails while maximized. Neither our handler nor DefWindowProc can consume it.
    if (msg == WM_WINDOWPOSCHANGED && !lParam) return 0;
    // Reset can synchronously dispatch activation and position messages as well
    // as WM_SIZE. Never re-enter game/rendering code through those callbacks.
    if (inst->resetInProgress && msg != WM_STYLECHANGING)
        return DefWindowProcA(wnd, msg, wParam, lParam);
    switch (msg)
    {
        // window focus/defocus
        case WM_ACTIVATE:
        {
            auto result = (LOWORD(wParam) == WA_INACTIVE) ?
                DefWindowProcA(wnd, msg, wParam, lParam) : // don't pause game on defocus
                CallWindowProcA(inst->oriWindowProc, wnd, msg, wParam, lParam);

            // handle automatic pause/resume
            if (inst->gameState == Playing_Game)
            {
                switch(LOWORD(wParam))
                {
                    case WA_INACTIVE:
                        if (Settings().Bool("MAIN.PauseOnFocusLoss") && !inst->IsMainMenuVisible())
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
                        if (Settings().Bool("MAIN.ResumeOnFocus") && inst->autoPauseExecuted &&
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
            return DefWindowProcA(wnd, msg, wParam, lParam);
        
        // restore proper handling of ShowCursor
        case WM_SETCURSOR:
            return DefWindowProcA(wnd, msg, wParam, lParam);

        // do not send keyboard events to the inactive window
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (!HasFocus(wnd))
                return DefWindowProcA(wnd, msg, wParam, lParam); // bypass the game
            
            // Alt+Enter and Ctrl+Enter: Windowed <-> Borderless
            const bool alt = (lParam & (1L << 29)) != 0;
            const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (wParam == VK_RETURN && (alt || control || inst->enterShortcutDown))
            {
                if (!(lParam & (1L << 30)) && !inst->enterShortcutDown)
                    inst->WindowToggleFullscreen();
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
                return DefWindowProcA(wnd, msg, wParam, lParam); // bypass the game
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
            if (inst->cursorClipped)
            {
                ClipCursor(NULL);
                inst->cursorClipped = false;
            }
            return 0;

        case WM_GETMINMAXINFO:
        {
            auto limits = (MINMAXINFO*)lParam;
            limits->ptMinTrackSize = inst->SizeFromClient(Resolution_Min);
            // a window as big as the screen keeps its frame outside it
            limits->ptMaxTrackSize.x = max(limits->ptMaxTrackSize.x, inst->windowSize.x);
            limits->ptMaxTrackSize.y = max(limits->ptMaxTrackSize.y, inst->windowSize.y);
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
            inst->WindowCalculateGeometry();
            if (inst->d3dDevice)
                CallWindowProcA(inst->oriWindowProc, wnd, WM_SIZE, SIZE_RESTORED,
                    MAKELPARAM(inst->windowSizeClient.x, inst->windowSizeClient.y));
            inst->SaveConfig();
            inst->MouseUpdate(true);
            return 0;

        // minimize, maximize, restore
        case WM_SIZE:
            if (!inst->windowUpdating && !inst->resetInProgress && !inst->sizing &&
                wParam != SIZE_MINIMIZED && wParam != SIZE_MAXHIDE && LOWORD(lParam) && HIWORD(lParam))
                CallWindowProcA(inst->oriWindowProc, wnd, msg, wParam, lParam); // inform the game
            return DefWindowProcA(wnd, msg, wParam, lParam); // call default as otherwise maximization will not work correctly on later Windows versions

        // a setting was changed by an editor, a preset or an overlay
        case WM_APPLY_SETTINGS:
            inst->ApplySettings();
            return 0;

        // moved to a monitor of another scale: keep the client size, the game's resolution
        case WM_DPICHANGED:
        {
            if (inst->windowMode != WindowMode::Windowed || IsZoomed(wnd))
                return 0;
            auto suggested = reinterpret_cast<const RECT*>(lParam);
            auto frame = ::WindowStyle::FrameOf(inst->WindowStyle(), inst->WindowStyleEx(), HIWORD(wParam));
            SetWindowPos(wnd, nullptr, suggested->left, suggested->top,
                inst->windowSizeClient.x + frame.Width(), inst->windowSizeClient.y + frame.Height(),
                SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }

        // position or size changed
        case WM_WINDOWPOSCHANGED:
        {
            if (inst->windowUpdating || inst->resetInProgress || IsIconic(wnd))
                return DefWindowProcA(wnd, msg, wParam, lParam);

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
                    return DefWindowProcA(wnd, msg, wParam, lParam);
                inst->windowSizeClient = { client.right, client.bottom };
                if (inst->windowMode == WindowMode::Windowed && !IsZoomed(wnd))
                    inst->windowSizeWindowed = inst->windowSizeClient;
                inst->WindowCalculateGeometry();
                inst->WindowUpdateTitle();
                if (!inst->sizing) inst->SaveConfig();
            }

            // DefWindowProc generates WM_SIZE/WM_MOVE. The game's handler must not
            // run a second time or resize the camera while a Reset is in progress.
            return DefWindowProcA(wnd, msg, wParam, lParam);
        }
    }

    return CallWindowProcA(inst->oriWindowProc, wnd, msg, wParam, lParam);
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

    // keep the cursor inside the window while playing; release it only when it was ours
    if (hasFocus || force)
    {
        if (!hasFocus || sizing || IsMainMenuVisible() || !Settings().Bool("MAIN.ConfineCursor"))
        {
            if (cursorClipped)
            {
                ClipCursor(NULL);
                cursorClipped = false;
            }
        }
        else
        {
            ClipCursor(&rect);
            cursorClipped = true;
        }
    }
}

#include "dxhandler.h"

CIniReader iniReader("");
int CDxHandler::nResetCounter = 0;
int CDxHandler::nCurrentWidth = 0, CDxHandler::nCurrentHeight = 0;
int CDxHandler::nNonFullWidth = 600, CDxHandler::nNonFullHeight = 450, CDxHandler::nNonFullPosX = 0, CDxHandler::nNonFullPosY = 0;
bool CDxHandler::bFullMode = false, CDxHandler::bRequestFullMode = false, CDxHandler::bRequestNoBorderMode = false;
bool CDxHandler::bChangingLocked = false;
HMENU CDxHandler::hMenuWindows = NULL;
WNDPROC CDxHandler::wndProcOld = NULL;
bool CDxHandler::bIsInputExclusive = false;
bool CDxHandler::bCursorStatus = true;
bool CDxHandler::bGameMouseInactive = false;
bool CDxHandler::bCtrlAltLastState = false;
bool CDxHandler::bAltEnterLastState = false;
bool CDxHandler::bShiftEnterLastState = false;
bool CDxHandler::bCtrlEnterLastState = false;
bool CDxHandler::bStopRecursion = false;
bool CDxHandler::bSizingLoop = false;
IDirect3D8** CDxHandler::pIntDirect3DMain;
IDirect3DDevice8** CDxHandler::pDirect3DDevice;
GameDxInput** CDxHandler::pInputData;
bool* CDxHandler::bMenuVisible;
HWND* CDxHandler::hGameWnd;
DisplayMode** CDxHandler::pDisplayModes;

HRESULT(__stdcall *CDxHandler::oldReset)(LPDIRECT3DDEVICE8 pDevice, void* pPresentationParameters);
HRESULT(__stdcall *CDxHandler::oldSetViewport)(LPDIRECT3DDEVICE8 pDevice, CONST D3DVIEWPORT8* pViewport);
void(*CDxHandler::CPostEffectsDoScreenModeDependentInitializations)();
void(*CDxHandler::CPostEffectsSetupBackBufferVertex)();
void(*CDxHandler::CMBlurMotionBlurOpen)(RwCamera*);
int(*CDxHandler::DxInputGetMouseState)(int a1);
void(*CDxHandler::ReinitializeRw)(int a1);
int(*CDxHandler::RwEngineGetCurrentVideoMode)();
bool(*CDxHandler::RwRasterDestroy)(RwRaster* pRaster);
RwRaster*(*CDxHandler::RwRasterCreate)(int32_t nWidth, int32_t nHeight, int32_t nDepth, int32_t nFlags);
RwCamera*(*CDxHandler::RwCameraClear)(RwCamera* pCamera, void* pColor, int32_t nClearMode);
RwCamera** CDxHandler::pRenderCamera;
RsGlobalType* CDxHandler::RsGlobal;
uint32_t CDxHandler::RwD3D8AdapterInformation_DisplayMode;
uint32_t CDxHandler::CamCol;
uint32_t CDxHandler::HookParams;
uint32_t CDxHandler::HookDirect3DDeviceReplacerJmp;
uint32_t CDxHandler::WSFixHook;
bool* CDxHandler::bBlurOn;
bool CDxHandler::bInGame3VC = false;
bool CDxHandler::bInGameSA = false;
bool CDxHandler::bResChanged = false;
bool CDxHandler::bWindowed = true;
bool CDxHandler::bUseMenus = true;
bool CDxHandler::bUseBorder = true;
SHELLEXECUTEINFOA CDxHandler::ShExecInfo = { 0 };
char CDxHandler::lpWindowName[MAX_PATH];

std::tuple<int32_t, int32_t> GetDesktopRes()
{
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    int32_t DesktopResW = info.rcMonitor.right - info.rcMonitor.left;
    int32_t DesktopResH = info.rcMonitor.bottom - info.rcMonitor.top;
    return std::make_tuple(DesktopResW, DesktopResH);
}

void CDxHandler::ProcessIni(void)
{
    bUseMenus = iniReader.ReadInteger("MAIN", "ShowMenu", 1) != 0;
    bUseBorder = iniReader.ReadInteger("MAIN", "ShowBorder", 1) != 0;
}

template<class D3D_TYPE>
HRESULT CDxHandler::HandleReset(D3D_TYPE* pPresentationParameters, void* pSourceAddress)
{
    if (bWindowed)
    {
        CDxHandler::AdjustPresentParams(pPresentationParameters);
    }

    CDxHandler::nResetCounter++;

    bool bInitialLocked = CDxHandler::bChangingLocked;
    if (!bInitialLocked) CDxHandler::StoreRestoreWindowInfo(false);

    bool bOldRecursion = CDxHandler::bStopRecursion;
    CDxHandler::bStopRecursion = true;
    HRESULT hRes = oldReset(*pDirect3DDevice, pPresentationParameters);
    CDxHandler::bStopRecursion = bOldRecursion;

    if (!bInitialLocked) CDxHandler::StoreRestoreWindowInfo(true);

    if (SUCCEEDED(hRes))
    {
        int nModeIndex = RwEngineGetCurrentVideoMode();
        (*CDxHandler::pDisplayModes)[nModeIndex].nWidth = pPresentationParameters->BackBufferWidth;
        (*CDxHandler::pDisplayModes)[nModeIndex].nHeight = pPresentationParameters->BackBufferHeight;
    }

    return hRes;
}

template<class D3D_TYPE>
void CDxHandler::AdjustPresentParams(D3D_TYPE* pParams)
{
    if (!bWindowed) return;

    bool bOldRecursion = bStopRecursion;

    if (hMenuWindows == NULL && (bInGame3VC || bInGameSA))
    {
        hMenuWindows = CreateMenu();
        AppendMenuA(hMenuWindows, MF_STRING, (WORD)&hMenuWindows, "CoordsManager");
    }

    pParams->Windowed = TRUE;

    pParams->FullScreen_PresentationInterval = 0;
    pParams->FullScreen_RefreshRateInHz = 0;
    pParams->EnableAutoDepthStencil = TRUE;
    pParams->BackBufferFormat = D3DFMT_A8R8G8B8;

    //pParams->MultiSampleType = (D3DMULTISAMPLE_TYPE)8;

    if (pParams->MultiSampleType > 0) {
        pParams->SwapEffect = D3DSWAPEFFECT_DISCARD;
    }

    DWORD dwWndStyle = GetWindowLong(*hGameWnd, GWL_STYLE);

    auto[nMonitorWidth, nMonitorHeight] = GetDesktopRes();

    nCurrentWidth = (int)pParams->BackBufferWidth;
    nCurrentHeight = (int)pParams->BackBufferHeight;

    RsGlobal->MaximumWidth = pParams->BackBufferWidth;
    RsGlobal->MaximumHeight = pParams->BackBufferHeight;

    HMENU hMenuSet = NULL;

    if (bRequestFullMode || (nCurrentWidth == nMonitorWidth && nCurrentHeight == nMonitorHeight))
    {
        dwWndStyle &= ~WS_OVERLAPPEDWINDOW;

        pParams->BackBufferWidth = nMonitorWidth;
        pParams->BackBufferHeight = nMonitorHeight;

        bFullMode = true;
        bRequestFullMode = false;
    }
    else
    {
        if (!bUseBorder)
        {
            dwWndStyle &= ~WS_OVERLAPPEDWINDOW;
        }
        else
        {
            dwWndStyle |= WS_OVERLAPPEDWINDOW;
            hMenuSet = bUseMenus ? hMenuWindows : NULL;
        }
        bFullMode = false;
    }

    RECT rcClient = { 0, 0, pParams->BackBufferWidth, pParams->BackBufferHeight };
    AdjustWindowRectEx(&rcClient, dwWndStyle, hMenuSet != NULL, GetWindowLong(*hGameWnd, GWL_EXSTYLE));

    int nClientWidth = rcClient.right - rcClient.left;
    int nClientHeight = rcClient.bottom - rcClient.top;

    bOldRecursion = bStopRecursion;
    bStopRecursion = true;

    SetWindowLong(*hGameWnd, GWL_STYLE, dwWndStyle);
    SetMenu(*hGameWnd, hMenuSet);

    if (hMenuSet)
    {
        rcClient.bottom = 0x7FFF;
        SendMessage(*hGameWnd, WM_NCCALCSIZE, FALSE, (LPARAM)&rcClient);
        nClientHeight += rcClient.top;
    }

    bStopRecursion = bOldRecursion;

    if (nClientWidth > nMonitorWidth)
        nClientWidth = nMonitorWidth;

    if (nClientHeight > nMonitorHeight)
        nClientHeight = nMonitorHeight;

    if (!bFullMode)
    {
        RECT rcWindow;
        GetWindowRect(*hGameWnd, &rcWindow);

        nNonFullWidth = nCurrentWidth;
        nNonFullHeight = nCurrentHeight;
        nNonFullPosX = rcWindow.left;
        nNonFullPosY = rcWindow.top;
    }

    bOldRecursion = bStopRecursion;
    bStopRecursion = true;
    SetWindowPos(*hGameWnd, HWND_NOTOPMOST, 0, 0, nClientWidth, nClientHeight, SWP_NOACTIVATE | (bFullMode ? 0 : SWP_NOMOVE));
    bStopRecursion = bOldRecursion;

    GetClientRect(*hGameWnd, &rcClient);

    pParams->BackBufferWidth = rcClient.right;
    pParams->BackBufferHeight = rcClient.bottom;
    pParams->hDeviceWindow = *hGameWnd;
    bResChanged = true;
}

void CDxHandler::ToggleFullScreen(void)
{
    int nModeIndex = RwEngineGetCurrentVideoMode();

    if (bFullMode)
    {
        SetWindowPos(*hGameWnd, NULL, nNonFullPosX, nNonFullPosY, nNonFullWidth, nNonFullHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    }
    else
    {
        auto[nMonitorWidth, nMonitorHeight] = GetDesktopRes();

        bRequestFullMode = true;

        SetWindowPos(*hGameWnd, NULL, 0, 0, nMonitorWidth, nMonitorHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    }
    bResChanged = true;
}

void CDxHandler::StoreRestoreWindowInfo(bool bRestore)
{
    static HMENU hStoredMenu = NULL;
    static int nStoredWidth = 0, nStoredHeight = 0, nStoredPosX = 0, nStoredPosY = 0;
    static DWORD dwStoredFlags = 0;

    RECT rcWindowRect;

    if (!bRestore)
    {
        hStoredMenu = GetMenu(*hGameWnd);
        dwStoredFlags = GetWindowLong(*hGameWnd, GWL_STYLE);

        GetWindowRect(*hGameWnd, &rcWindowRect);
        nStoredWidth = rcWindowRect.right - rcWindowRect.left;
        nStoredHeight = rcWindowRect.bottom - rcWindowRect.top;
        nStoredPosX = rcWindowRect.left;
        nStoredPosY = rcWindowRect.top;

        bChangingLocked = true;
    }
    else
    {
        bChangingLocked = false;

        GetWindowRect(*hGameWnd, &rcWindowRect);
        int nCurWidth = rcWindowRect.right - rcWindowRect.left;
        int nCurHeight = rcWindowRect.bottom - rcWindowRect.top;

        if (nCurWidth != nStoredWidth || nCurHeight != nStoredHeight || GetWindowLong(*hGameWnd, GWL_STYLE) != dwStoredFlags || GetMenu(*hGameWnd) != hStoredMenu)
        {
            bool bOldRecursion = bStopRecursion;
            bStopRecursion = true;

            SetWindowLong(*hGameWnd, GWL_STYLE, dwStoredFlags);
            SetMenu(*hGameWnd, hStoredMenu);
            SetWindowPos(*hGameWnd, NULL, nStoredPosX, nStoredPosY, nStoredWidth, nStoredHeight, SWP_NOACTIVATE | SWP_NOZORDER);

            bStopRecursion = bOldRecursion;
        }
    }
}

void CDxHandler::AdjustGameToWindowSize(void)
{
    RECT rcClient;
    GetClientRect(*hGameWnd, &rcClient);

    int nModeIndex = RwEngineGetCurrentVideoMode();
    if (*pDisplayModes)
    {
        bool bSizeChanged = ((*pDisplayModes)[nModeIndex].nWidth != rcClient.right || (*pDisplayModes)[nModeIndex].nHeight != rcClient.bottom);

        if (bSizeChanged) {
            (*pDisplayModes)[nModeIndex].nWidth = rcClient.right;
            (*pDisplayModes)[nModeIndex].nHeight = rcClient.bottom;

            RwCameraClear(*pRenderCamera, (void*)CamCol, 2);
        }

        if (!bFullMode) {
            RECT rcWindow;
            GetWindowRect(*hGameWnd, &rcWindow);

            if (rcWindow.left != 0 && rcWindow.top != 0)
            {
                nNonFullPosX = rcWindow.left;
                nNonFullPosY = rcWindow.top;
            }
        }
    }
}

void CDxHandler::MainCameraRebuildRaster(RwCamera* pCamera)
{
    if (pCamera == *pRenderCamera)
    {
        IDirect3DSurface8* pSurface;
        D3DSURFACE_DESC stSurfaceDesc;

        (*pDirect3DDevice)->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pSurface);
        pSurface->GetDesc(&stSurfaceDesc);
        pSurface->Release();

        int nModeIndex = RwEngineGetCurrentVideoMode();

        if ((*CDxHandler::pDisplayModes)[nModeIndex].nWidth != (int)stSurfaceDesc.Width || (*CDxHandler::pDisplayModes)[nModeIndex].nHeight != (int)stSurfaceDesc.Height)
        {
            (*CDxHandler::pDisplayModes)[nModeIndex].nWidth = (int)stSurfaceDesc.Width;
            (*CDxHandler::pDisplayModes)[nModeIndex].nHeight = (int)stSurfaceDesc.Height;
        }

        int nGameWidth = (*CDxHandler::pDisplayModes)[nModeIndex].nWidth;
        int nGameHeight = (*CDxHandler::pDisplayModes)[nModeIndex].nHeight;

        *(int*)RwD3D8AdapterInformation_DisplayMode = nGameWidth;
        *(int*)(RwD3D8AdapterInformation_DisplayMode + 4) = nGameHeight;

        if (pCamera->frameBuffer && (pCamera->frameBuffer->nWidth != nGameWidth || pCamera->frameBuffer->nHeight != nGameHeight))
        {
            RwRasterDestroy(pCamera->frameBuffer);
            pCamera->frameBuffer = NULL;
        }

        if (!pCamera->frameBuffer)
        {
            pCamera->frameBuffer = RwRasterCreate(nGameWidth, nGameHeight, 32, rwRASTERTYPECAMERA);
        }

        if (pCamera->zBuffer && (pCamera->zBuffer->nWidth != nGameWidth || pCamera->zBuffer->nHeight != nGameHeight))
        {
            RwRasterDestroy(pCamera->zBuffer);
            pCamera->zBuffer = NULL;
        }

        if (!pCamera->zBuffer)
        {
            pCamera->zBuffer = RwRasterCreate(nGameWidth, nGameHeight, 0, rwRASTERTYPEZBUFFER);
        }
    }
}

void CDxHandler::ActivateGameMouse(void)
{
    if (bGameMouseInactive)
    {
        bGameMouseInactive = false;
        DxInputCreateDevice(true);
    }
}

LRESULT APIENTRY CDxHandler::MvlWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == (WORD)&hMenuWindows)
        {
            static bool bIsCMopen = false;
            static auto cb = [](HWND hwnd, LPARAM lParam) -> BOOL
            {
                char text[25];
                GetWindowTextA(hwnd, text, 25);
                if (strncmp(text, "Coords Manager", strlen("Coords Manager")) == 0)
                {
                    SetForegroundWindow(hwnd);
                    bIsCMopen = true;
                    return FALSE;
                }
                return TRUE;
            };

            EnumWindows(cb, 0);

            if (!bIsCMopen)
            {
                char* szFilePath = (char*)iniReader.GetIniPath().c_str();
                *strrchr(szFilePath, '\\') = '\0';
                strcat(szFilePath, "\\III.VC.SA.CoordsManager.exe");

                ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
                ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                ShExecInfo.hwnd = NULL;
                ShExecInfo.lpVerb = NULL;
                ShExecInfo.lpFile = szFilePath;
                ShExecInfo.lpParameters = "";
                ShExecInfo.lpDirectory = NULL;
                ShExecInfo.nShow = SW_SHOWNORMAL;
                ShExecInfo.hInstApp = NULL;
                ShellExecuteExA(&ShExecInfo);
                //WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
            }
        }
        break;
    case WM_LBUTTONUP:
        if (!*bMenuVisible && IsCursorInClientRect())
        {
            ActivateGameMouse();
        }
        break;
    case WM_STYLECHANGING:
        if (bChangingLocked)
        {
            STYLESTRUCT* pStyleInfo = (STYLESTRUCT*)lParam;
            pStyleInfo->styleOld = pStyleInfo->styleNew;
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        bSizingLoop = true;
        return 0;
    case WM_EXITSIZEMOVE:
    case WM_SIZE:
    case WM_SIZING:
    case WM_WINDOWPOSCHANGED:
    case WM_WINDOWPOSCHANGING:
    {
        if (uMsg == WM_EXITSIZEMOVE)
        {
            bSizingLoop = false;
        }

        if (uMsg == WM_WINDOWPOSCHANGING && bChangingLocked)
        {
            WINDOWPOS* pPosInfo = (WINDOWPOS*)lParam;
            pPosInfo->flags = SWP_NOZORDER | SWP_NOSIZE | SWP_NOREPOSITION | SWP_NOREDRAW | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOACTIVATE;
        }

        if (!bChangingLocked && !bStopRecursion && bWindowed)
        {
            AdjustGameToWindowSize();
        }

        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if ((GetKeyState(VK_MENU) & 0x8000) && (wParam == VK_CONTROL || wParam == VK_RETURN))
        {
            return 0;
        }
        break;
    case WM_SYSCOMMAND:
        if (wParam == SC_KEYMENU)
        {
            return 0;
        }
        break;
    }

    return CallWindowProc(wndProcOld, hwnd, uMsg, wParam, lParam);
}

HRESULT __stdcall ResetSA(LPDIRECT3DDEVICE8 pDevice, D3DPRESENT_PARAMETERS_D3D9* pPresentationParameters) {
    return CDxHandler::HandleReset(pPresentationParameters, nullptr);
}

HRESULT __stdcall Reset(LPDIRECT3DDEVICE8 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    return CDxHandler::HandleReset(pPresentationParameters, nullptr);
}

HRESULT __stdcall SetViewport(LPDIRECT3DDEVICE8 pDevice, CONST D3DVIEWPORT8* pViewport) {
    bool bInitialLock = CDxHandler::bChangingLocked;

    if (!bInitialLock) CDxHandler::StoreRestoreWindowInfo(false);
    HRESULT hres = CDxHandler::oldSetViewport(*CDxHandler::pDirect3DDevice, pViewport);
    if (!bInitialLock) CDxHandler::StoreRestoreWindowInfo(true);

    return hres;
}

void CDxHandler::Direct3DDeviceReplaceSA(void)
{
    if (*pDirect3DDevice != NULL)
    {
        UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)*pDirect3DDevice));
        if (!oldReset)
        {
            oldReset = (HRESULT(__stdcall *)(LPDIRECT3DDEVICE8 pDevice, void* pPresentationParameters))(*(uint32_t*)&pVTable[16]);
            oldSetViewport = (HRESULT(__stdcall *)(LPDIRECT3DDEVICE8 pDevice, CONST D3DVIEWPORT8* pViewport))(*(uint32_t*)&pVTable[47]);
        }

        injector::WriteMemory(&pVTable[16], &ResetSA, true);
        injector::WriteMemory(&pVTable[47], &SetViewport, true);
    }
}


void CDxHandler::Direct3DDeviceReplace(void)
{
    if (*pDirect3DDevice != NULL)
    {
        UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)*pDirect3DDevice));
        if (!oldReset)
        {
            oldReset = (HRESULT(__stdcall *)(LPDIRECT3DDEVICE8 pDevice, void* pPresentationParameters))(*(uint32_t*)&pVTable[14]);
            oldSetViewport = (HRESULT(__stdcall *)(LPDIRECT3DDEVICE8 pDevice, CONST D3DVIEWPORT8* pViewport))(*(uint32_t*)&pVTable[40]);
        }

        injector::WriteMemory(&pVTable[14], &Reset, true);
        injector::WriteMemory(&pVTable[40], &SetViewport, true);
    }
}

void CDxHandler::InjectWindowProc(void)
{
    if (*hGameWnd != NULL && wndProcOld == NULL)
    {
        wndProcOld = (WNDPROC)GetWindowLong(*hGameWnd, GWL_WNDPROC);
        SetWindowLong(*hGameWnd, GWL_WNDPROC, (LONG)CDxHandler::MvlWndProc);
    }
}

void CDxHandler::RemoveWindowProc(void)
{
    if (*hGameWnd != NULL && wndProcOld != NULL)
    {
        if (GetWindowLong(*hGameWnd, GWL_WNDPROC) == (LONG)CDxHandler::MvlWndProc)
        {
            SetWindowLong(*hGameWnd, GWL_WNDPROC, (LONG)wndProcOld);
            wndProcOld = NULL;
        }
    }
}

void CDxHandler::DxInputCreateDevice(bool bExclusive)
{
    static GUID guidDxInputMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };

    if (!*pInputData || !(*pInputData)->pInput) return;

    if (bExclusive == true && bGameMouseInactive)
    {
        bExclusive = false;
    }

    bIsInputExclusive = bExclusive;

    DWORD dwCooperativeLevel = DISCL_FOREGROUND;

    if (bExclusive) dwCooperativeLevel |= DISCL_EXCLUSIVE;
    else dwCooperativeLevel |= DISCL_NONEXCLUSIVE;

    if ((*pInputData)->pInputDevice)
    {
        (*pInputData)->pInputDevice->Unacquire();
    }
    else
    {
        (*pInputData)->pInput->CreateDevice(guidDxInputMouse, &(*pInputData)->pInputDevice, NULL);
        if (!(*pInputData)->pInputDevice) return;

        (*pInputData)->pInputDevice->SetDataFormat((LPCDIDATAFORMAT)0x67EE9C);
    }

    (*pInputData)->pInputDevice->SetCooperativeLevel(*hGameWnd, dwCooperativeLevel);
    (*pInputData)->pInputDevice->Acquire();
}

bool CDxHandler::IsCursorInClientRect(void)
{
    POINT cursorPos;
    RECT rcClient;

    GetClientRect(*hGameWnd, &rcClient);

    if (GetForegroundWindow() == *hGameWnd && GetCursorPos(&cursorPos) && ScreenToClient(*hGameWnd, &cursorPos))
    {
        if (cursorPos.x >= rcClient.left && cursorPos.x < rcClient.right && cursorPos.y >= rcClient.top && cursorPos.y < rcClient.bottom)
        {
            return true;
        }
    }

    return false;
}

int CDxHandler::ProcessMouseState(void)
{
    static bool bOnce = false;
    if (!bOnce)
    {
        auto[nMonitorWidth, nMonitorHeight] = GetDesktopRes();
        if (!bFullMode)
        {
            nNonFullPosX = static_cast<int>(((float)nMonitorWidth / 2.0f) - ((float)nNonFullWidth / 2.0f));
            nNonFullPosY = static_cast<int>(((float)nMonitorHeight / 2.0f) - ((float)nNonFullHeight / 2.0f));
            SetWindowPos(*hGameWnd, NULL, nNonFullPosX, nNonFullPosY, nNonFullWidth, nNonFullHeight, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        else
        {
            bRequestFullMode = true;
            SetWindowPos(*hGameWnd, NULL, 0, 0, nMonitorWidth, nMonitorHeight, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        bResChanged = true;
        bOnce = true;
    }

    static Fps _fps;
    _fps.update();

    sprintf(lpWindowName, "%s | %dx%d @ %d fps", RsGlobal->AppName, RsGlobal->MaximumWidth, RsGlobal->MaximumHeight, _fps.get());
    SetWindowTextA(*hGameWnd, lpWindowName);

    bool bShowCursor = true;
    bool bForeground = (GetForegroundWindow() == *hGameWnd);

    bool bCtrlAltCurState = bForeground && ((GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000));
    bool bAltEnterCurState = bForeground && ((GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_RETURN) & 0x8000));
    bool bShiftEnterCurState = bForeground && ((GetKeyState(VK_SHIFT) & 0x8000) && (GetKeyState(VK_RETURN) & 0x8000));
    bool bCtrlEnterCurState = bForeground && ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_RETURN) & 0x8000));

    static DWORD dwLastCheck = 0;

    if (dwLastCheck + 1000 < GetTickCount())
    {
        dwLastCheck = GetTickCount();

    }
    else
    {
        if (bInGame3VC)
        {
            auto UpdateWSFixData = injector::GetBranchDestination(WSFixHook, true);
            if (UpdateWSFixData != nullptr)
                injector::cstd<void()>::call(UpdateWSFixData);
        }

        if (bResChanged)
        {
            if (bInGame3VC)
            {
                if (*bBlurOn)
                    CMBlurMotionBlurOpen(*pRenderCamera);
            }

            if (bInGameSA)
            {
                CPostEffectsSetupBackBufferVertex();
                //CPostEffectsDoScreenModeDependentInitializations();
            }

            bResChanged = false;
        }
    }

    if (!bFullMode && !bShiftEnterLastState && bShiftEnterCurState)
    {
        if (bUseMenus && !bUseBorder)
            bUseMenus = true;
        else
            bUseMenus = !bUseMenus;

        bUseBorder = true;
        SetMenu(*hGameWnd, bUseMenus ? hMenuWindows : NULL);
        iniReader.WriteInteger("MAIN", "ShowMenu", bUseMenus, true);
    }
    else
    {
        if (!bFullMode && !bCtrlEnterLastState && bCtrlEnterCurState)
        {
            bUseBorder = !bUseBorder;
            DWORD dwWndStyle = GetWindowLong(*hGameWnd, GWL_STYLE);
            if (!bUseBorder)
            {
                dwWndStyle &= ~WS_OVERLAPPEDWINDOW;
            }
            else
            {
                dwWndStyle |= WS_OVERLAPPEDWINDOW;
            }
            SetWindowLong(*hGameWnd, GWL_STYLE, dwWndStyle);
            iniReader.WriteInteger("MAIN", "ShowBorder", bUseBorder, true);
        }
    }
    bShiftEnterLastState = bShiftEnterCurState;
    bCtrlEnterLastState = bCtrlEnterCurState;

    if (!bAltEnterLastState && bAltEnterCurState)
    {
        ToggleFullScreen();
    }

    bAltEnterLastState = bAltEnterCurState;

    if (*bMenuVisible)
    {
        bShowCursor = !IsCursorInClientRect();
        bGameMouseInactive = false;
    }
    else
    {
        if (!bCtrlAltLastState && bCtrlAltCurState)
        {
            if (bGameMouseInactive)
            {
                DxInputCreateDevice(true);
                bGameMouseInactive = false;
            }
            else
            {
                DxInputCreateDevice(false);
                bGameMouseInactive = true;
            }
        }

        if (!bGameMouseInactive && GetForegroundWindow() == *hGameWnd)
        {
            bShowCursor = false;
        }
    }

    bCtrlAltLastState = bCtrlAltCurState;

    if (bCursorStatus != bShowCursor)
    {
        ShowCursor(bShowCursor);
        bCursorStatus = bShowCursor;
    }

    if ((!*bMenuVisible && bGameMouseInactive) || (*bMenuVisible && !IsCursorInClientRect()))
    {
        return 0;
    }

    return 1;
}

void __declspec(naked) CDxHandler::HookDirect3DDeviceReplacer(void)
{
    static HRESULT hRes;
    static bool bOldRecursion;
    static bool bOldLocked;

    _asm pushad

    bOldRecursion = bStopRecursion;
    bStopRecursion = true;

    InjectWindowProc();
    AdjustPresentParams((D3DPRESENT_PARAMETERS*)HookParams);

    bOldLocked = bChangingLocked;
    if (!bOldLocked)  StoreRestoreWindowInfo(false);
    RemoveWindowProc();
    bChangingLocked = true;

    _asm popad
    _asm call[ecx + 3Ch]
        _asm mov hRes, eax
    _asm pushad

    bChangingLocked = bOldLocked;
    InjectWindowProc();
    if (!bOldLocked)  StoreRestoreWindowInfo(true);

    Direct3DDeviceReplace();
    bStopRecursion = bOldRecursion;

    _asm popad
    _asm cmp eax, ebp
    _asm jmp HookDirect3DDeviceReplacerJmp
}

void __declspec(naked) CDxHandler::HookDirect3DDeviceReplacerSA(void)
{
    static HRESULT hRes;
    static bool bOldRecursion;
    static bool bOldLocked;

    _asm pushad

    bOldRecursion = bStopRecursion;
    bStopRecursion = true;

    InjectWindowProc();
    AdjustPresentParams((D3DPRESENT_PARAMETERS_D3D9*)HookParams);

    bOldLocked = bChangingLocked;
    if (!bOldLocked) StoreRestoreWindowInfo(false);
    RemoveWindowProc();
    bChangingLocked = true;

    _asm popad
    _asm call[edx + 40h]
        _asm mov hRes, eax
    _asm pushad

    bChangingLocked = bOldLocked;
    InjectWindowProc();
    if (!bOldLocked) StoreRestoreWindowInfo(true);

    Direct3DDeviceReplaceSA();
    bStopRecursion = bOldRecursion;

    _asm popad
    _asm test eax, eax
    _asm jmp HookDirect3DDeviceReplacerJmp
}

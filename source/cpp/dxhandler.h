#pragma once
#include "misc.h"

class CDxHandler
{
public:
    template<class D3D_TYPE>
    static HRESULT HandleReset(D3D_TYPE* pPresentationParameters, void* pSourceAddress = 0);
    template<class D3D_TYPE>
    static void AdjustPresentParams(D3D_TYPE* pParams);
    static void ToggleFullScreen(void);
    static void StoreRestoreWindowInfo(bool bRestore);
    static void AdjustGameToWindowSize(void);
    static void MainCameraRebuildRaster(RwCamera* pCamera);
    static void Direct3DDeviceReplace(void);
    static void Direct3DDeviceReplaceSA(void);
    static void InjectWindowProc(void);
    static void RemoveWindowProc(void);
    static void ActivateGameMouse(void);
    static LRESULT APIENTRY MvlWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void DxInputCreateDevice(bool bExclusive);
    static bool IsCursorInClientRect(void);
    static int ProcessMouseState(void);
    static void HookDirect3DDeviceReplacer(void);
    static void HookDirect3DDeviceReplacerSA(void);
    static void SetupHooksVC(void);
    static void SetupHooksIII(void);
    static void SetupHooksSA(void);
    static void ProcessIni(void);
    static HRESULT(__stdcall *oldReset)(LPDIRECT3DDEVICE8 pDevice, void* pPresentationParameters);
    static HRESULT(__stdcall *oldSetViewport)(LPDIRECT3DDEVICE8 pDevice, CONST D3DVIEWPORT8* pViewport);

    static bool bIsInputExclusive;
    static bool bCursorStatus;
    static bool bGameMouseInactive;
    static bool bCtrlAltLastState;
    static bool bAltEnterLastState;
    static bool bShiftEnterLastState;
    static bool bCtrlEnterLastState;

    static int nCurrentWidth, nCurrentHeight;
    static int nNonFullWidth, nNonFullHeight, nNonFullPosX, nNonFullPosY;
    static bool bFullMode, bRequestFullMode, bRequestNoBorderMode;
    static bool bChangingLocked;
    static bool bSizingLoop;

    static IDirect3D8** pIntDirect3DMain;
    static IDirect3DDevice8** pDirect3DDevice;
    static GameDxInput** pInputData;
    static bool* bMenuVisible;
    static HWND* hGameWnd;

    static int nResetCounter;

    static HMENU hMenuWindows;

    static bool bNeedsInitialSwitch;

    static WNDPROC wndProcOld;

    static DisplayMode** pDisplayModes;
    static bool bStopRecursion;

    static SHELLEXECUTEINFOA ShExecInfo;
    static char lpWindowName[MAX_PATH];
    static void(*CPostEffectsDoScreenModeDependentInitializations)();
    static void(*CPostEffectsSetupBackBufferVertex)();
    static void(*CMBlurMotionBlurOpen)(RwCamera*);
    static int(*DxInputGetMouseState)(int a1);
    static void(*ReinitializeRw)(int a1);
    static int(*RwEngineGetCurrentVideoMode)();
    static RwCamera*(*RwCameraClear)(RwCamera* pCamera, void* pColor, int32_t nClearMode);
    static bool(*RwRasterDestroy)(RwRaster* pRaster);
    static RwRaster*(*RwRasterCreate)(int32_t nWidth, int32_t nHeight, int32_t nDepth, int32_t nFlags);
    static RwCamera** pRenderCamera;
    static bool* bBlurOn;
    static RsGlobalType* RsGlobal;
    static uint32_t RwD3D8AdapterInformation_DisplayMode;
    static uint32_t CamCol;
    static uint32_t HookParams;
    static uint32_t HookDirect3DDeviceReplacerJmp;
    static bool bInGame3VC;
    static bool bInGameSA;
    static bool bResChanged;
    static bool bWindowed;
    static bool bUseMenus;
    static bool bUseBorder;
};
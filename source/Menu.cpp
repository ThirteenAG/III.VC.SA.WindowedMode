module;
#include "stdafx.h"
#include "Bindings.h"

module WindowedMode;

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
    inst->bindings.selectVideoMode(modeIndex);
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
    const auto result = inst->bindings.changeVideoMode(modeIndex);
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

bool WindowedMode::IsMainMenuVisible() const
{
    return *bindings.menuActive;
}

void WindowedMode::SwitchMainMenu(bool show)
{
    if (show == IsMainMenuVisible()) return;
    if (bindings.requestMenuOpen)
    {
        if (show) bindings.requestMenuOpen();
        else bindings.requestMenuClose();
    }
    else
    {
        *bindings.menuOpen = show;
        *bindings.menuClose = !show;
    }
}

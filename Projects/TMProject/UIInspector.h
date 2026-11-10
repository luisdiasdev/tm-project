#pragma once

#include <Windows.h>
#include <d3d9.h>

namespace UIInspector
{
    bool Initialize(HWND hWnd, IDirect3DDevice9* pd3dDevice);
    void Shutdown();
    void Render();
    bool HandleInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnDeviceLost();
    void OnDeviceReset(IDirect3DDevice9* pd3dDevice);

    void Toggle();
    bool IsEnabled();
    void ToggleShowAll();
    bool IsShowAllEnabled();
    void ToggleFreeze();
    bool IsFrozen();

    // Optional helper for fail-loud FindControl diagnostics.
    void LogMissingControl(unsigned int dwID);
}

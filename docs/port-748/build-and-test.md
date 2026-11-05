# Build, Run, Capture, Click

## Build

From the workspace root (`E:\wyd\test-setup` on the reference machine):

```powershell
msbuild tmproject\TM.sln /t:Build /p:Configuration=Debug /p:Platform=x86
```

Notes:

- Build the **solution**, not the `.vcxproj`: the project's `IncludePath` uses
  `$(SolutionDir)Dependencies\Directx\Include`.
- The solution defines `Debug|x86`, not `Debug|Win32`. Use `Platform=x86`.
- Output: `tmproject\..\client_748\TMProject_d.exe` (set in `TMProject.vcxproj`).
- A running client locks the EXE. Kill it before building:
  `Get-Process TMProject_d | Stop-Process -Force`.
- `C1853` (PCH mismatch) usually means stale PCH from another toolset; run
  `/t:Rebuild`.
- The debug console (`CreateConsole()`) may be enabled in `NewApp.cpp`; it does
  not affect the game window.

## Run

```powershell
Start-Process .\client_748\TMProject_d.exe -WorkingDirectory .\client_748
```

The client reads `config.txt` and writes `WYD.log` in the working directory.
`client_748\config.txt` currently uses `[WINDOW] 1`, `[RES] 3` (800x600) and
`[CLASSIC] 0`. Note `NewApp.cpp` currently forces `g_UIVer = 2`.

## Capture the window

`PrintWindow()` returns a black image for this D3D9 window. Use
`Graphics.CopyFromScreen` over the window rect instead, after
`SetForegroundWindow` + a short settle delay.

`docs/tools/capture_and_click.ps1` does launch → wait → optional click →
capture. Example:

```powershell
powershell -File docs\tools\capture_and_click.ps1 `
  -Exe ..\client_748\TMProject_d.exe `
  -WorkingDir ..\client_748 `
  -ClickX 377 -ClickY 498 `
  -Out modal.png
```

The script crops to the window rect, so it can include the title bar. On the
reference machine the window is `816x639` at `(0,0)`; the game client area is
`800x600` starting roughly at `(8,31)` (26px title bar).

## Clicking UI in a test

The coordinates below are **screen coordinates** with the window at `(0,0)`,
for the 800x600 client. They were measured from a capture and confirmed with a
temporary `LOG_WRITELOG` in `TMSelectServerScene::OnControlEvent`.

| Target | Logical rect | Click |
|---|---|---|
| Server-select Close (`TMB_SELSERVER_EXIT`, 5637) | panel (272,236) + (80,225), 50x23 → (352,461)-(402,484) | `(377, 498)` |
| Server-select Connect (`TMB_SELSERVER_OK`, 4620) | (286,461)-(336,484) | `(311, 498)` |
| Modal Confirm | box origin + (20,77), 88x21 | depends on box position |
| Modal Cancel | box origin + (117,77), 88x21 | depends on box position |

Hold the mouse button for ~150-180 ms before releasing; the client processes
input per frame and a zero-length synthetic click can be missed.

## Verifying the click reached the scene

Temporarily add at the top of `TMSelectServerScene::OnControlEvent`:

```cpp
LOG_WRITELOG(">> OnControlEvent id=%u event=%u\n", idwControlID, idwEvent);
```

Then `WYD.log` should contain e.g. `>> OnControlEvent id=5637 event=0`.
Remove the probe before committing.

## Manual visual baseline

| Screen | Expected |
|---|---|
| Login / server select | WYD II FC logo, two stone panels "Server"/"Channel", one "World 5v" row, Connect/Close. |
| Close modal | Dark etched 256x128 frame, "Information", "Want to exit the game?", Confirm/Cancel with button art. |
| Char select | Not yet verified end-to-end. |
| Field | Not yet verified (server required). |

Compare against the original `client_748\WYD.exe` screenshots when in doubt.

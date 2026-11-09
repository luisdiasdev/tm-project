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
`client_748\config.txt` currently uses `[WINDOW] 1`, `[RES] 3`, and
`[CLASSIC] 0`. Note `NewApp.cpp` forces `g_UIVer = 2`.

## `config.txt`

`config.txt` is the 748 source of truth (`LoadConfigTxt()` in `NewApp.cpp`,
ported from `FUN_0055bc0a` in `WYD.exe`). Every key is optional; the defaults
in `TMConfig` (`NewApp.h`) match the original. Unknown keys are ignored.

| Key | Effect |
|---|---|
| `[VERSION]` | Parsed, ignored (same as `WYD.exe`). |
| `[RES]` | 1-based index into the 748 resolution table (default `2`). |
| `[ANIMATION]` | Clamped to 2; the original's animation-LOD flags are not ported, and the smooth-skinning level is then forced to 1. |
| `[SOUND]` | `m_nSound` (0 disables sound init). Default `1`. |
| `[MUSIC]` | `m_nMusic` (0 disables BGM init). Default `0`. |
| `[SERVER]` | Parsed, ignored. |
| `[BRIGHT]` | Gamma brightness, clamped to 0..100. Default `50`. |
| `[CURSOR]` | `1` = system cursor, any other value = WYD cursors. |
| `[DEMO]` | Non-zero enables the UI fade (see `server-select.md`). |
| `[WINDOW]` | `-1` keeps the command-line mode; `0` = fullscreen, `1` = windowed. |
| `[CLASSIC]` | Read, but the 748 client forces UI2 right after reading it. |
| `[CAMERAROTATE]` | Any positive value enables camera rotation. |
| `[DXT]` | Any positive value disables DXT (the original stores the inverse). |
| `[KEYTYPE]` | `g_nKeyType`. |
| `[CAMERAVIEW]` | `m_nCameraView` (stored; the port has no consumer for it yet). |

The resolution table matches `WYD.exe`: entries 1..5 are 16-bit
(640x480, 800x600, 1024x768, 1280x1024, 1600x1200) and 6..11 are 32-bit
(the same list plus 3200x2400). If `EnumDisplaySettings` does not report the
selected mode, the client falls back to entry 2 (800x600x16). Most modern
drivers do not expose 16-bit modes, so `[RES] 3` still produces an 800x600
window, exactly like the original.

Test account: `admin` / `admin`. On the server list, pick the **4th** entry
(`World SV-…`); the rows above it are "No Server." placeholders. Clicking
Connect without selecting the channel row shows "Select server and server
group." and stops.

## Capture the window

`PrintWindow()` returns a black image for this D3D9 window. Use
`Graphics.CopyFromScreen` over the window rect instead, after
`SetForegroundWindow` + a short settle delay.

`docs/tools/client_drive.ps1` wraps launch / click / type / capture / login in
one command:

```powershell
powershell -File docs\tools\client_drive.ps1 -Action launch -Restart -Out artifacts\01-serverselect.png
powershell -File docs\tools\client_drive.ps1 -Action login  -Out artifacts\02-selchar.png
powershell -File docs\tools\client_drive.ps1 -Action click  -X 70 -Y 233 -Out artifacts\03-create.png
```

It crops to the window rect, so captures can include the title bar. On the
reference machine the window is `816x639` at `(0,0)`; the game client area is
`800x600` starting roughly at `(8,31)` (26px title bar). `-X`/`-Y` are
window-relative, so they match the image when the window is at `(0,0)`.

`docs/tools/capture_and_click.ps1` is the older single-shot (launch → optional
click → capture) variant and still works.

## Clicking UI in a test

The coordinates below are **screen coordinates** with the window at `(0,0)`,
for the 800x600 client. They were measured from a capture and confirmed with a
temporary `LOG_WRITELOG` in `TMSelectServerScene::OnControlEvent`.

| Target | Logical rect | Click |
|---|---|---|
| Server-select Close (`TMB_SELSERVER_EXIT`, 5637) | panel (272,236) + (80,225), 50x23 → (352,461)-(402,484) | `(377, 498)` |
| Server-select Connect (`TMB_SELSERVER_OK`, 4620) | (286,461)-(336,484) | `(311, 498)` |
| Server group row (`World SV`) | list row 1 | `(340, 330)` |
| Channel row (4th entry) | list rows; placeholders above | `(470, 360)` |
| Login account edit (`TME_LOGIN_ID`) | login box | `(430, 296)` |
| Login button (`TMB_LOGIN_OK`, 4609) | (324,342)-(395,361) | `(360, 351)` |
| Char-select Create button (panel 5654) | (30,190), 86x21 | `(70, 233)` |
| Char-select Exit button (panel 5672) | (30,230), 86x21 | `(70, 273)` |
| Create screen: class previews | four models on the stage | bodies around `y = 320`; e.g. `(200, 330)` |
| Create screen: create window | shown after picking a class, at `(514,35)` | `(602, 441)` Create, `(690, 441)` Cancel |
| Modal Confirm | box origin + (20,77), 88x21 | depends on box position |
| Modal Cancel | box origin + (117,77), 88x21 | depends on box position |

Hold the mouse button for ~150-180 ms before releasing; the client processes
input per frame and a zero-length synthetic click can be missed.

The create screen only appears after clicking one of the four class previews:
that click is what shows panel `1542` and fills the class name, description,
stats and bars.

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
| Char select | Hall with Create/Delete/Exit; selecting a character shows the info panel (name/class/level/gold/stats) and Connect/Cancel. No password modal until a delete is confirmed. |
| Field | Verified: Armia map renders, player name `Agate`, mobs/NPC names, click-to-move and attack. HUD: chat list bottom-left, inventory on `i`, auto-mode icons `318/319`, minimap area mostly empty, 755 `Att`/`Def` labels still misplaced at bottom centre. |

Compare against the original `client_748\WYD.exe` screenshots when in doubt.

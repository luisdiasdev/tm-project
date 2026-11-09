# Tools

These are the checks used during the 748 port. They are read-only unless noted.

## `ui_inventory.py`

```powershell
cd tmproject
python docs\tools\ui_inventory.py --assets ..\client_748
python docs\tools\ui_inventory.py --assets ..\client_748 --source-refs
```

- Walks `SelServerScene2.bin`, `SelCharScene2.bin`, `FieldScene2.bin` with the
  current `UIBinary.h` sizes and requires exact EOF.
- Counts control types, reports IDs missing from `ResourceControl.h`, and
  verifies that every positive texture set used by the scene exists in
  `UITextureSetList.txt`.
- With `--source-refs`, scans the matching `.cpp` for `FindControl()` and
  `idwControlID ==` references and reports values not present in the scene.
  Dynamic controls (message box, IME) and 755-only controls are expected in
  this list.
- Warns about the malformed set 198 (`ItemCount: 5`, four rows).

Generated reports live in `docs/port-748/reference/`.

## `rewrite_control_ids.py`

Dry-run by default:

```powershell
python docs\tools\rewrite_control_ids.py `
  --source Projects/TMProject/TMFieldScene.cpp `
  --scene-bin ..\client_748\UI\FieldScene2.bin
```

It only rewrites when the old value is absent from the target scene and a
`TM`+name alias exists whose value is present. It preserves the file's raw
bytes by reading/writing latin-1 and removes the C++ `u` suffix from replaced
numbers. Review the printed list and `git diff` before `--write`.

Do **not** run it on a file that is valid UTF-8 with intentional non-ASCII
text unless you have verified the round-trip.

## `restore_non_utf8.py`

```powershell
python docs\tools\restore_non_utf8.py --file Projects/TMProject/TMFieldScene.cpp
```

Restores lines whose raw bytes were replaced by U+FFFD, using
`git show HEAD:<file>`. Run from the repo root; use `--dry-run` first.
`TMFieldScene.cpp` should have 13 raw non-ASCII bytes on 9 lines.

## `generate_serverlist.py`

Writes the client's `serverlist.bin` (7040 bytes) and `sn.bin` (143 bytes) in
the exact format `BASE_InitializeServerList()` / `InitServerName()` expect.

```powershell
cd tmproject

# default: the shipped group layout, every address 127.0.0.1, written into
# ..\client_748 (previous files are kept as *.bak)
python docs\tools\generate_serverlist.py

# a custom local setup, written elsewhere
python docs\tools\generate_serverlist.py --out ..\client_748 `
  --group-names "LOCAL" --servers 2 --ip 127.0.0.1

# decode and inspect the pair currently in a folder
python docs\tools\generate_serverlist.py --out ..\client_748 --dump
```

- `serverlist.bin` is 10 groups x 11 entries x 64 bytes. Entry 0 of a group is
  the user-count URL and doubles as the "group exists" flag, so it must be
  non-empty or the client hides the group.
- Every 64-byte record is scrambled with the fixed table from
  `BASE_InitializeServerList()`; **all** slots must be written, including the
  unused ones, or they decode to garbage.
- `sn.bin` is 11 names of 9 bytes plus 11 little-endian int32 values. The
  int32 entry `i` selects serverlist group `value - 1`, and the displayed name
  is `names[i + 1]` (so `names[0]` is a non-empty sentinel, "Reserved").
- The top server row is the client's aggregate/admit group: it borrows one
  server from each lower group. The defaults keep the shipped per-group counts
  (reserved 2, MALECH 6, SEPHIRA 4, ALASTAR 4, NOVATO 1) for that reason.
- `--config` accepts a JSON file with a `groups` list; the first element is the
  hidden reserved group 0. See the script docstring for an example.
- The tool re-encodes the shipped 748 pair byte-for-byte in a round-trip test.

## `client_drive.ps1`

Launches, clicks, types, captures and runs the login sequence against the 748
client. This is the tool to use for UI checks; `capture_and_click.ps1` below is
the older single-shot variant.

```powershell
cd tmproject

# start a fresh client and grab the server-select screen
powershell -File docs\tools\client_drive.ps1 -Action launch -Restart -Out artifacts\01-serverselect.png

# server row -> channel row -> Connect -> admin/admin -> Login (waits for the UI)
powershell -File docs\tools\client_drive.ps1 -Action login -Out artifacts\02-selchar.png

# click Create, capture the result
powershell -File docs\tools\client_drive.ps1 -Action click -X 70 -Y 233 -Out artifacts\03-create.png

# type into the focused control, capture the result
powershell -File docs\tools\client_drive.ps1 -Action type -Text "Testchar" -Out artifacts\04-typed.png

powershell -File docs\tools\client_drive.ps1 -Action kill
```

- Actions: `launch`, `capture`, `click`, `type`, `login`, `kill`.
- Defaults: `-Exe ..\client_748\TMProject_d.exe`, working directory = its folder.
- `-X`/`-Y` are **window-relative**, so they match the captured image when the
  window is at `(0,0)`; the client area starts ~26 px below the window top.
- The click is held for `-HoldMilliseconds` (default 180) because the client
  polls input per frame and a zero-length click can be missed.
- `-Text` is a SendKeys string: `"admin{TAB}admin"`, `"{ENTER}"`, `"{ESC}"`.
- Captures use `CopyFromScreen` over the window rect (`PrintWindow` returns
  black for this D3D9 window), so keep the game unobstructed on top.
- `-Action launch` waits `-WaitSeconds` (default 8) before the optional capture.
- The login sequence uses the `admin`/`admin` test account and the 4th server
  entry (`World SV-...`); the rows above it are placeholders. If the login panel
  is still up after the sequence, run the reported retry click.

## `capture_and_click.ps1`

```powershell
powershell -File docs\tools\capture_and_click.ps1 `
  -Exe ..\client_748\TMProject_d.exe `
  -WorkingDir ..\client_748 `
  -ClickX 377 -ClickY 498 `
  -Out modal.png
```

Launches, waits, brings the window forward, optionally clicks (held for
`-HoldMilliseconds`), and captures the window rect with `CopyFromScreen`.
`PrintWindow` is not used because it returns black for this D3D9 window.
Coordinates are relative to the window rect; with the window at `(0,0)` they
match the captured image. Add ~26 px to logical client Y for the title bar.

Kill stale clients before building:
`Get-Process TMProject_d | Stop-Process -Force`.

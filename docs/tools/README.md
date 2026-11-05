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

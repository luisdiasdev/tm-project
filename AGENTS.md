# AGENTS.md — TMProject / WYD 748

## Mission

`Projects/TMProject` is a decompiled WYD client aimed at the 755/760 client data.
This branch (`test/748`) ports it to the **748** client assets in
`../client_748` (English 748, 2007-04). Target: a playable 748 client on the
existing D3D9 codebase.

Read `docs/README.md` first; it indexes the detailed notes and the audit tools.

## Build

From the repository root (`tmproject/`):

```powershell
msbuild TM.sln /t:Build /p:Configuration=Debug /p:Platform=x86
```

(From the workspace root: `msbuild tmproject\TM.sln ...`.)

- Use the **solution** and `Platform=x86` (the solution has no `Win32`).
- Output is `../client_748/TMProject_d.exe` (configured in the vcxproj).
- Kill a running `TMProject_d.exe` first, otherwise the link fails with
  `LNK1168`.
- If you see `C1853` (PCH from a different compiler), do a `/t:Rebuild`.
- Runtime log: `../client_748/WYD.log`.

## Rules that save time

1. **748 uses the old `TM*` control-ID namespace.** `ResourceControl.h` contains
   both the `TM*` (748/older) and plain (755/760) constants. Scene binaries and
   `FindControl()` calls must agree on the same namespace.
2. **UI scene `.bin` formats are compatible.** `FieldScene2.bin`,
   `SelCharScene2.bin`, `SelServerScene2.bin` parse exactly to EOF with the
   current `UIBinary.h` structs. Do not rewrite those structs.
3. **Texture sets are 0..456 in 748.** `GetUITextureSet()` only bounds-checks
   against `MAX_UI_TEXTURE_SET_LIST` (600), so a missing set returns an
   empty/zeroed entry and silently renders nothing. Verify indices exist.
4. **Much of the 748 UI is built at runtime.** The server window and message
   box are code-generated from texture sets, not fully described by the scene
   binaries. When a panel is white/blank, look for the original constructor.
5. **`TMFieldScene.cpp` is not UTF-8.** It contains 13 raw non-ASCII bytes on
   9 lines. Text editors/agents can replace them with U+FFFD. Use
   `docs/tools/restore_non_utf8.py` if that happens; prefer byte-safe edits.
6. **Do not port the renderer to D3D8.** The 748 delta that matters is data,
   IDs, structs and packets, not the graphics API. The project is D3D9.
7. **`g_UIVer` is forced to 2** in `NewApp.cpp`. 748's "new UI" branch is what
   we target; the original 748 still keeps a classic branch (`!= 2`) for some
   screens.

## Current state

- Login/server-select: pixel-matched to the original 748 client.
- Message box: 748 background/button textures restored.
- Field: IDs remapped and null-guarded; **not yet tested in-game** because no
  server was running.
- Character select: mostly already uses 748 IDs; a few 755-only account-lock
  controls are absent.

See `docs/port-748/remaining-work.md` for the next steps and
`docs/port-748/pitfalls.md` before editing UI code.

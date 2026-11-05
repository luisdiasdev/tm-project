# Pitfalls

## Client variants

- Target is `../client_748` (English, `WYD.exe` 2007-04-23).
- `../client_748_br` is a different 748 build (2007-03-16). Its UI scene bins are
  much larger (`SelServerScene2.bin` 2716 bytes vs 1352, `FieldScene2.bin`
  84716 vs 35404, `SelCharScene2.bin` 12756 vs 4820). Do not copy UI files
  between the two.
- `../client` is 755/760 data; it is the upstream source of the current code,
  not a target.

## Texture sets

- `GetUITextureSet()` only bounds-checks `0..600`, not the loaded set count.
  Missing sets return a zeroed entry and render nothing. Check the set exists.
- `-1` and `-2` are special; `< -2` means `abs(value)`.
- Set `-397` / `397` is `NewUI_Minibar_Panel`, an icon strip, **not** the server
  window frame. The frame is `391`.
- 748 has **no `NUI\` textures**. `UITextureList.bin` records 300+ are empty.
  Any 755 "new UI" set/texture index in that range will render nothing.
- `UITextureSetList.txt` has a data bug: set `198` (`chat_sel2`) declares
  `ItemCount: 5` but only has 4 coordinate rows. The C loader's `fscanf` fails
  on the next section header and resynchronizes, so parsing continues, but the
  5th coordinate is uninitialized. Do not rely on item index 4 of set 198.
- 748 has 446 sets but a max index of 456; max index alone is not a safe range.

## Encoding

- `Projects/TMProject/TMFieldScene.cpp` is **not valid UTF-8**. It contains 13
  raw non-ASCII bytes on 9 lines, including a `'\xA8'` character literal and
  Portuguese comments.
- Text editors / agent edit tools that assume UTF-8 replace those bytes with
  `EF BF BD` (U+FFFD), changing program semantics. If that happens, run
  `docs/tools/restore_non_utf8.py` (it restores the exact bytes from
  `git show HEAD:<file>`).
- Prefer the byte-preserving scripts in `docs/tools/` for bulk edits to that
  file.

## Dynamic controls

The audit tool will report `FindControl(4617)` (message box), `4628` (IME) and
similar as "missing from the scene". They are created at runtime by `TMScene`
or by the scene code, so their absence from the `.bin` is expected. Do not
"fix" them by binding to unrelated IDs.

## ID mapping traps

- `TM` + name is a heuristic, not a total mapping.
- A `TM` value may exist in a scene but mean a different logical control there.
  The rewriter only maps when the target value is present **and** the source
  reference is absent; for cross-scene code, verify semantics manually.
- Plain constants used as *message values* (`B_QUIT`, `B_CREATE_ID`, ...) are
  compared against `m_pMessageBox->m_dwMessage`. They are currently consistent
  with each other (both use the 755 value), so leave them together or convert
  both sides in one pass. Do not convert only one side.

## Build / runtime

- Kill `TMProject_d.exe` before building (`LNK1168`).
- Use the solution with `Platform=x86`; `Debug|Win32` is invalid for the `.sln`.
- `C1853` means stale PCH; `/t:Rebuild`.
- `PrintWindow` returns black for this D3D9 window; capture with
  `CopyFromScreen` over the window rect.
- Synthetic clicks must be held ~150 ms; the client polls input per frame.
- The window rect includes the title bar (26 px on the reference machine).
  Logical client coordinates and captured image coordinates differ by that
  offset.

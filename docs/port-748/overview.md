# 748 Port — Overview

## Goal

`Projects/TMProject` is a faithful decompilation of a 755/760-era WYD client.
The workspace also contains the original 748 clients:

| Folder | What it is |
|---|---|
| `../client_748` | English 748 client (`WYD.exe` 2007-04-23). **Target assets.** |
| `../client_748_br` | Brazilian 748 client (2007-03-16). Different UI bins. Do not mix. |
| `../client` | 755/760-era client used by upstream TMProject. |
| `../release` | TMSRV/DBSRV server binaries (not documented here yet). |

The port keeps the existing D3D9 renderer and adapts data readers, control IDs
and runtime-built UI to the 748 asset set.

## Branch and commits

Branch: `test/748`

| Commit | Purpose |
|---|---|
| `114d1f0` | Boot fixes: `strdef.bin` size, packed login packet, 256-entry `curse.bin`, `UITextureList.bin` path, select-server IDs, output to `client_748`. |
| `38e44a7` | Rebind 229 field-scene control references to the 748 `TM*` namespace. |
| `8a8a221` | Rebuild the 748 server window at runtime; null-guard 755-only field controls. |
| `2892f2d` | Remove the invalid D3D9 render-target allocation (clears `WYD.log` error). |
| `0c9455b` | 748 message-box frame/button textures and layout. |

## Method that worked

1. **Treat every asset reader as a format question.** Compute expected record
   sizes and verify the file consumes exactly to EOF. This proved the UI scene
   structs were already compatible and found the `strdef`/`curse` sizes.
2. **Diff the control-ID namespaces before touching rendering.** 748 scenes use
   the `TM*` constants; the 755 code uses `65k+` IDs. A static audit of
   `FindControl()` calls versus the actual scene binary turned "the UI is
   broken" into a finite list of bindings.
3. **Reproduce visually.** Launch `TMProject_d.exe`, capture only its window
   (see `build-and-test.md`), and compare against the original `client_748/WYD.exe`.
   `PrintWindow` returns black for this D3D9 window; use `CopyFromScreen`.
4. **Use the original binary as the oracle.** When the asset bin did not explain
   a panel (white box, missing modal), recover the constructor in `WYD.exe` with
   REA/Ghidra, then port the literal constants and positions. See
   `../reverse-engineering/rea-workflow.md`.
5. **Guard, do not guess.** Controls that genuinely do not exist in 748 are
   guarded instead of being mapped to a random control with a similar ID.

## Big findings

- The 748 scene binaries parse exactly with the current `UIBinary.h` records:
  709 field controls, 85 character-select controls, 22 server-select controls.
- `SelServerScene.bin` is **not** a full classic scene in this distribution. It
  contains only 5 controls plus a copyright string; the classic window is built
  entirely by code.
- The 748 client uses UI version `2` (`DAT_005b892c == 2`) and builds the server
  and channel windows from texture sets `391/392/393/394/396`; the scene's
  `TMP_SELSERVER` panel is only a white fill behind them.
- The message box uses set `164` (background) and `165` (button plates) in 748.
  The 755 code used set `501`, which does not exist in 748.
- The D3D9 render-target textures were invalid (`D3DUSAGE_RENDERTARGET` +
  `D3DPOOL_MANAGED`) and unused; they are now a no-op.

## What is proven vs. assumed

Proven by execution and screenshots:

- Client boots, logs in UI renders, server-select window matches the original,
  Close modal matches the original.
- `WYD.log` is clean after the render-target fix.

Not yet proven:

- Entering the field (no server was running during the port).
- Character-select edge cases (account-lock panel).
- Field HUD interactions and the ~80 controls that do not exist in 748.

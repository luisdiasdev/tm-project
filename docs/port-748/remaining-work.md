# Remaining Work

## 1. Test character select and field entry

Nothing was listening on the server ports during the port, so the verified
surface stops at the server-select screen and its modal.

Next steps:

1. Start `TMSrv`/`DBSrv` (see `../release`) or point the client at a running
   server. `NewApp.cpp` currently hardcodes `127.0.0.1`; check
   `serverlist.bin`/`sn.bin` if that is wrong.
2. Run `client_748\TMProject_d.exe`, select the server, log in, and capture
   character select.
3. Enter the field and capture the HUD; check `WYD.log` for new errors.

Anything that crashes is a missing null guard or an unported ID; see below.

## 2. Character select

`TMSelectCharScene.cpp` already uses 748 IDs for the main screen. The audit
still reports these references that are absent from `SelCharScene2.bin`:

| ID | Meaning | Expected action |
|---:|---|---|
| `4617` | message box | created at runtime; fine |
| `4628` | IME button | 755-only; guard |
| `65886`, `65887` | account-lock OK/Cancel | 755-only; guard |
| `66433..66435` | char-select extras | not in 748; guard/disable |

The account-lock panel path should be made conditional so a missing panel does
not dereference null.

## 3. Field HUD

Already done:

- 229 control bindings remapped to `TM*`.
- Null guards for controls absent from `FieldScene2.bin`: ccmode, PK, chat
  back/chat-select, auto-skill children, cargo/inventory page grids, RPS,
  party buttons, daily quest, etc.
- Fallback to `TMG_INV_GRID` / `TMG_CARGO_GRID` when the 755 multi-page grids
  are absent.

Still open:

- `docs/port-748/reference/missing-source-bindings.txt` lists ~118 references
  that do not resolve. Most are 755-only features; a few may have 748
  equivalents under different names. Resolve those by comparing the 748 scene
  controls (type/position/parent) with the intended feature, or disable the
  feature for 748.
- The field bin contains 16 IDs without names in `ResourceControl.h`
  (`318, 319, 802..811, 1857, 1862, 1863, 1864`). Name them in the header if
  they matter; otherwise leave numeric.
- Page switching code still assumes four inventory pages / three cargo pages.
  With the single 748 grid it should be verified or hidden.
- Chat-select (`90113/90114/90128/90129`) is a 755 feature. It is guarded but
  the chat type button is gone in 748; decide whether to hide the selector.
- Party auto, kingdom UI, item-mix pages, auto-trade, daily quest and PK were
  not present in 748. Keep them disabled rather than faking IDs.

## 4. Data readers not yet audited

The UI/string/curse/login readers were verified. Still worth the same
"size == count * record size, parse to EOF" check:

- `object.bin`, `ItemList.bin`, `Itemname.bin`, `InitItem.bin`,
  `SkillData.bin`, `minimap.dat`, `serverlist.bin`/`sn.bin`,
  `Mesh/*`, `Effect/*`, `Env/*`.
- Packet structs used in the field. Only `MSG_CNFAccountLogin` was packed for
  748 so far; other 748 structs may need packing or field changes.

## 5. UI polish / known oddities

- `ReadUIString()` uses `%s`, so multi-word 748 UI strings such as
  `C.C Mode(MC)` are truncated to `C.C`. Fix by reading the rest of the line if
  the exact string matters.
- `SMessageBox::SetMessage()` still splits long text at 34 characters; the 748
  box is 232 px wide, so wrapping may need tuning.
- `SelServerScene.bin` is a stub; do not spend time trying to make the classic
  path work unless a full classic scene from another 748 build is available.
- `g_UIVer` is forced to `2` in `NewApp.cpp`. Making it config-driven is
  possible, but only UI2 assets were validated.

## 6. Regression checklist

After each UI change:

1. Build Debug|x86.
2. Launch and capture the server-select screen; compare with
   `client_748/WYD.exe`.
3. Click Close and capture the modal (background + button plates).
4. Confirm `WYD.log` has no new errors.
5. If you touched `TMFieldScene.cpp`, verify the non-UTF-8 byte count still
   matches `HEAD` (13 bytes / 9 lines).

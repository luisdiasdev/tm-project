# Remaining Work

## 1. Test character select and field entry

Login reaches TMSrv/DBSrv and the account response transitions to character
select. Character select now initializes and is usable; field entry is still
untested.

Next steps:

1. Start `TMSrv`/`DBSrv` with their own `run` directories as the working
   directory, or the account lookup fails with "Conta inexistente".
   `NewApp.cpp` currently hardcodes `127.0.0.1`; check
   `serverlist.bin`/`sn.bin` if that is wrong.
2. Run `client_748\TMProject_d.exe`, select the **4th** server entry
   (`World SV-…`), log in with `admin`/`admin`, and capture character select.
3. Enter the field and capture the HUD; check `WYD.log` for new errors.

Anything that crashes is a missing null guard or an unported ID; see below.

## 2. Character select

Fixed: `InitializeScene` dereferenced 755-only controls. The account-lock
dialog and the select/back panel do not exist in `SelCharScene2.bin`, and the
delete-password dialog exists under different IDs.

| 755 ID | 748 ID | Handling |
|---:|---:|---|
| `66432`, `66447`, `66128`, `66132`, `66433..66435`, `66437..66446` | — | account lock, 755-only. The bindings stay null and every use is guarded; an account without the dialog is treated as unlocked. |
| `65885`, `65889` | `626`, `627` | delete-password panel/edit, bound with a fallback so the 748 panel is hidden at init and keeps its `.bin` position. |
| `65886`, `65887` | `1024`, `921` | delete-password confirm/cancel, handled by the same code as the 755 IDs. |
| `1054274` | — | select/back panel. `VisibleSelectCreate` only repositions the Create/Delete/Exit panels when it exists; in 748 they keep their `.bin` positions. |
| `1544` | — | create-character background 2, already guarded. |
| `4617`, `4628` | — | message box / IME, created at runtime or 755-only; no crash. |

Note: `626/627/1024/921` are the same values the field scene uses for its
gold-input dialog (`TMP_INPUT_GOLD` / `TME_INPUT_GOLD` / `TMB_IG_OK2` /
`TMB_IG_CANCEL2`). In `SelCharScene2` they mean "Enter Password", so do not
reuse the field names when editing this scene.

Verified at 800x600: login → character select renders, selecting a character
shows the info panel, Delete → Confirm opens "Enter Password", Cancel closes it.

### Create-character screen

`VisibleSelectCreate(0)` only toggled the 755 create backgrounds
(`1537..1541`), which are absent from `SelCharScene2.bin`; the whole 748 create
UI is panel `1542` (name edit `4626`, description list `5671`, Create/Cancel).
The panel is positioned as the 755 window was and stays hidden until a class
preview is clicked, matching the 748 flow.

Clicking one of the four previews (`m_pSampleHuman`) now populates the panel:
class name (`TMT_CREATE_CLASS` 5666), character description from
`UI\chardesc<race>.txt` into `TMP_CREATE_CHAR_DESC` (5671), the Str/Int/Dex/Con
values (1562..1565) and the four bar widths (5667..5670). The values are the
per-class preview table the original client hardcodes; they are not derived from
the server. The description list renders correctly even though the scene stores
`5671` with width 0 — `LoadMsgText` copies the list width into each item at load
time and the empty width is not used for clipping.

Create (`TMB_CREATE_CHAR_OK` 1545, also Enter while the name edit is focused)
validates the name and sends `MSG_NewCharacter`; the server reply drives the
message panel (`0x11A`) or the character list refresh (`MSG_CNFNewCharacter`).
That path depends on the server and is not verified from the client side.

Still open:

- At 800x600 the window at `(514, 35)` overlaps the 4th class preview; decide
  whether to move the window or the previews.
- While the create window is visible the scene ignores mouse clicks on the 3D
  previews, so another class cannot be picked without cancelling first. This is
  inherited from the 755 code; check the original 748 client before changing it.

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

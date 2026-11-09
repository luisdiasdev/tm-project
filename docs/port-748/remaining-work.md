# Remaining Work

## 1. Character select and field entry

Login reaches TMSrv/DBSrv, character select is usable and entering the world
now works. Verified at 800x600: select `Agate`, click Connect, the field scene
loads, the HUD renders, click-to-move and attacking a mob work.

To reproduce:

1. Start `TMSrv`/`DBSrv` with their own `run` directories as the working
   directory, or the account lookup fails with "Conta inexistente".
   `NewApp.cpp` currently hardcodes `127.0.0.1`; check
   `serverlist.bin`/`sn.bin` if that is wrong.
2. Run `client_748\TMProject_d.exe`, select the **4th** server entry
   (`World SV-…`), log in with `admin`/`admin`.
3. Select a character and click Connect (window-relative ~`(582,416)` at
   800x600). `WYD.log` must reach `>> Init Field Scene::End`.

### The 0x114 crash chain (fixed)

`MSG_CNFCharacterLogin` is handled by `TMSelectCharScene`, which switches to
`TM_FIELD_STATE`; `TMFieldScene::InitializeScene()` then runs synchronously.
The 755-only UI assumptions crashed in this order:

| Crash | Cause | Fix |
|---|---|---|
| `m_pCCModeHpSte->SetText` | 748 has no C.C. mode dialog `66817..66824` | null-guarded; the `B_CCMODE_*` handlers are guarded too |
| `m_pSkillSecGrid2[i]->Empty` | second skill bar `94208..94219` absent; the guard checked the array, not the element | check `m_pSkillSecGrid2[i]` |
| `m_pGridInvList[0]->m_bDrawGrid` | inventory page grids `67072..67075` absent | per-element guard; a single grid (`TMG_INV_GRID`) is used |
| `SGridControl::AddItem` (login carry loop) | `m_pGridInvList[Page]` null | fall back to `m_pGridInv`, 9-column cell mapping |
| same for cargo | `m_pCargoGridList[Page]` null | fall back to `m_pCargoGrid` |
| `OnPacketCreateMob` `m_pMyHuman` | a modal `MessageBox` in `CItemMix::BASE_ReadMixList` pumped the socket queue while init was still running | no modal for the optional file; mobs are ignored until `m_pMyHuman` exists |
| `CItemMix::TakeItResource` / `CMission::TakeItResource` | `81921`/`81948`/`86017` absent | guards; 748 mix panel `1360` is used as fallback |
| `Bag_View` | `m_pInvPageBtn3/4`, `m_pJPNBag_Day1/2` absent | early return; page buttons are 755-only |
| `Affect_Main` | affect icon strips `90400`/`90369`/`475152` absent | early return after clearing `TMT_AFFECT_DESC` |
| `FrameMove` party loop | `m_pPartyList` (`475138`) absent | guard |
| click-to-move | `m_pChatBack` (`65943`) absent | treated as "click is not over the chat" |
| shortcut assignment | `m_pSkillSecGrid[i]` / `[2][i]` null | guards |

`Mixlist.bin` is not a 748 asset and the original `WYD.exe` has no reference to
it, so the reader now logs and returns instead of showing a modal box. Item-mix
recipes are therefore empty (see below).

### Things that are still 755-shaped

- **Inventory/cargo pages.** The runtime code still maps item positions as
  4 pages × 15 slots (inventory) and 3 pages × 40 slots (cargo). At init the
  unused page pointers are aliased to the single 748 grid so nothing crashes,
  but cell placement for runtime pickups/uses is still 5-column math. Fixing it
  needs a page/cell abstraction driven by `m_nColumnGridCount`.
- **Party UI.** 748 uses panel `1857`, list `1863`; those are bound as a
  fallback. There is no party button (`5742` is absent), so the window is only
  hidden at init and `Affect_Main`'s party refresh is skipped.
- **Affect display.** The 748 client shows affect text through
  `TMT_AFFECT_DESC` (`12834`) and has no icon strips. `Affect_Main` currently
  just clears the description.
- **Item mix.** 748 has panels `1360`, `6110`, `6145`, `6481`, `6432`, `6512`
  with a different grid layout (e.g. `1367..1372`). They are all hidden at
  init; the 755 `CItemMix` class only knows the `81921` layout.
- **Auto attack / C.C. buttons.** The original 748 field init looks up `318`
  and `319` (texture sets `455 game_mauto` / `456 game_sauto`, children of the
  skill panel `5745`) and sets them visible and pickable. `FUN_004539bc` in
  `WYD.exe` toggles them on `A`/`D`. The port does not bind them yet and
  `OnKeyAuto` is still a stub.
- **HUD layout.** The top-left status panel (`TMP_MAIN_INFO2`, 5723) is now
  visible again, and the bottom-centre strip uses the 748 `TMP_MAIN_INFO1`
  art/backdrop with the single 748 EXP bar (1171). The minimap area still only
  shows the server/NPC name. See sections below for the remaining control
  bindings.

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
- Top-left status panel restored: `TMP_MAIN_INFO2` (5723) is no longer hidden,
  so the HP/MP orbs, name, level and HP/MP/mount labels render. The HP orb keeps
  texture set 419 instead of the missing 755 set 538/545..547.
- Bottom-centre stat strip: `TMP_MAIN_INFO1` (5716) is positioned correctly and
  its background (5717) stays at `(0,0)`; Att/Def and the `current / next` EXP
  text are filled from `UpdateScoreUI`.
- Single 748 EXP bar (`TMP_EXP_PROGRESS`, 1171) bound in addition to the 755
  ten-segment array (which remains null in 748).
- Auto attack now starts disabled; the 748 `TMP_SKILL_SEL` (575) red line is no
  longer treated as the 755 auto-skill panel.
- Debug FPS text is off by default (`g_bDebugMsg = 0`) so it no longer covers
  the chat tabs; re-enable it locally when profiling.

Still open:

- `docs/port-748/reference/missing-source-bindings.txt` lists ~118 references
  that do not resolve. Most are 755-only features; a few may have 748
  equivalents under different names. Resolve those by comparing the 748 scene
  controls (type/position/parent) with the intended feature, or disable the
  feature for 748. The field-entry crash chain (section 1) covered the ones
  reached on login; inventory/trade/mix/party paths still assume the 755
  multi-page layouts.
- The field bin contains 16 IDs without names in `ResourceControl.h`
  (`318, 319, 802..811, 1857, 1862, 1863, 1864`). `318/319` are the 748
  auto-attack buttons and `1857/1863/1864` the 748 party window; name them in
  the header when they are wired up.
- Page switching code still assumes four inventory pages / three cargo pages.
  The missing page grids are aliased to `TMG_INV_GRID`/`TMG_CARGO_GRID` so the
  client does not crash, but item cell positions still use 5-column math.
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
5. Log in, select a character, enter the field and confirm
   `>> Init Field Scene::End` in `WYD.log`, then move, open the inventory
   (`i`) and attack a mob.
6. If you touched `TMFieldScene.cpp`, verify the non-UTF-8 byte count still
   matches `HEAD` (13 bytes / 9 lines). The file is not valid UTF-8; the
   `'\xA8'` literal and Portuguese comments must stay as raw bytes. If an
   editor rewrote them as U+FFFD, restore them from `HEAD` (the mapping is
   1:1: each raw byte became `EF BF BD`).

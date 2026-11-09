# 01 — Scope, assets and what the 2026 client actually does

## 1. The 2026 client is a layered hybrid, not a rewrite

This is the single most important scoping fact, and it is now verified with
**code xrefs and a full load chain**, not just RTTI strings. The shipped
`WYD.exe` (SHA-256 `a058262d…c50`) runs both systems.

### 1.1 The `.bin` scene path is live

- `FUN_005205f0` (0x5205f0) is the scene factory. It switches on the scene type
  and constructs real objects:

  | Case | Class | Vtable | Scene type |
  |---|---|---|---|
  | 0 | `TMFieldScene` | `0x88b79c` | 30000 (0x7530) |
  | 5 | `TMSelectCharScene` | `0x88c460` | 0x7531 |
  | 7 | `TMSelectServerScene` | `0x88c980` | 0x7534 |
  | 8 | `TMDemoScene` | — | 0x7535 |

  After construction it calls vtable `+0x4c` (`InitializeScene`).
- `TMFieldScene::InitializeScene` = `0x4e6000`. It logs
  `>> Init Field Scene::Start`, then calls `TMScene::LoadRC` (vtable `+0x3c` =
  `0x508430`) with `UI\FieldScene2.txt`.
- `LoadRC` creates the `SControlContainer`, **rewrites the extension to
  `.bin`**, and calls the container's `ReadRCBin` (vtable `+0x44`).
- The 2026 scene files parse with the 748 record structs:

  | File | Controls | Loaded? |
  |---|---:|---|
  | `FieldScene2.bin` | **1,354** (623 panels, 290 texts, 204 buttons, 185 grids, 32 listboxes, 13 progress, 7 edits) | yes, by `TMFieldScene` |
  | `SelCharScene2.bin` | **107** (55 texts, 26 buttons, 21 panels, 4 edits, 1 listbox) | yes, by `TMSelectCharScene` |
  | `SelServerScene2.bin` | 19 (the old 748 login/server window) | **no** |
  | `SelServerScene3.bin` | **2 panels** only | yes, by `TMSelectServerScene` |

- The char-select init `0x515700` loads `UI\SelCharScene2.txt`; the
  server-select init `0x51b110` loads `UI\SelServerScene3.txt`.

### 1.2 The `.pane` path is live

All panel loading goes through `FUN_0045de30` → `FUN_00482c10` (the documented
`.pane` loader). The concrete controllers are real, not dead RTTI:

| Controller | Init function | Vtable | `.pane` files |
|---|---|---|---|
| `PanelHUD` | `0x5ce5a0` | `0x896b10` (+0x10) | HUD, HudMyInfo, HudYouInfo |
| `PanelInventory` | `0x5d1eb0` | `0x8970bc` (+0x10) | Inventory |
| `PanelFieldScene` | `0x5c17c0` | `0x895c90` (+0x10) | FieldScene, MiniPopup |
| login panels | `0x51a700` | — | GameLogin, GameLogin2, GameLogin3 |
| char-select panels | `0x512c50` | — | CharSelect_Menu/Create/Password/SelInfo |
| `ServerSelectPopup` | `0x5e9930` | — | ServerSelectPopup |

### 1.3 What each screen actually uses

| Screen | `.bin` (SControl) | `.pane` (CGui) | Reality |
|---|---|---|---|
| Login / server select | `SelServerScene3.bin` (2 background panels) | GameLogin, GameLogin2, GameLogin3, ServerSelectPopup (and `itemPanel` rows) | **pane-only UI**; there is no `LoginScene` string |
| Character select | `SelCharScene2.bin` (107 controls) | CharSelect_Menu/Create/Password/SelInfo | **mixed** |
| Field | `FieldScene2.bin` (**1,354 controls, 185 grids**) | HUD, HudMyInfo/HudYouInfo, Inventory, chat, … | classic scene UI is still fully loaded and initialized; panes are an additional window layer |

The classic field UI is therefore **not vestigial data**; it is larger than the
748 scene and is created at scene start. The most likely model is a
config/branch where the classic controls are the "old UI" and the panes are the
"new UI"; both are shipped and both code paths are present. The exact selector
and the hiding of the replaced classic controls are still open (see
[`04-risks-and-decisions.md`](04-risks-and-decisions.md), U17). Our own
`g_UIVer` switch (forced to 2 in `NewApp.cpp`) is a convenient place to make
that choice, but it is **not proven** that 2026 uses that variable for it.

### 1.4 Why your search found only one reference

- The file's preferred image base is **0x400000**. Your IDA view is rebased by
  **+0x190000**: `0x676042`/`0xA1BA6C` are `0x4E6042`/`0x88BA6C` in the file.
- The code passes `UI\FieldScene2.txt`, never `FieldScene2.bin`. `LoadRC`
  rewrites the extension at runtime. Searching for `*.bin` strings finds
  nothing.
- Ghidra reports `XBUI\FieldScene2.txt` at `0x88BA6A` because the two bytes
  before the literal belong to an adjacent string. The path starts at
  `0x88BA6C`.
- There are **three** scene paths, not one:
  `UI\FieldScene2.txt`, `UI\SelCharScene2.txt`, `UI\SelServerScene3.txt`.
- `LoginScene` does not exist in the 2026 binary because the login UI is
  `GameLogin.pane` inside the server-select scene. The old
  `SelServerScene2.bin` (19 controls) is shipped but not loaded.
- `LoginScene.bin`, `LoginScene2.bin` and `SelServerScene2.bin` are present in
  the 2026 `UI/` folder but have **no code reference**. File presence is not
  evidence of use.
- 14 of the 111 texture sets used by `FieldScene2.bin` are absent from the
  shipped `UITextureSetList.txt` (347 sets, max index 565). The classic branch
  has data drift; another reason not to port the 2026 `.bin` scene wholesale.

**Plan consequence:** do not attempt a big-bang replacement of `TMScene` /
`SControl`, and do not treat the `.bin` scene as dead. Add a `Pane` layer with
its own panels, renderer, input router and asset resolver, and migrate windows
one at a time. Because the 2026 client itself keeps the classic branch, we can
also keep the 748 item grids while adopting panes for login, HUD and chat (see
the scope levers in [`03-plan-and-effort.md`](03-plan-and-effort.md#5-scope-levers)).

## 2. What ships in the 2026 data

All numbers verified against `../wydglobal2026` and
`docs/wyd-pane/decoded-json` on 2026-11-08. Reproduce them with
`python docs/wyd-pane/tools/pane_audit.py --client ../wydglobal2026`.

### 2.1 Panels

- 64 files under `UI/Panel/` (63 distinct; `ItemOptionStone` exists as both
  `.pane` and a plaintext `.panel` copy).
- 1,315 element records; **1,299 unique** if the duplicate is counted once.
- 280 KB total — panels are tiny.

Element type distribution (all shipped files):

| Type | Count | Notes |
|---|---:|---|
| IconButton | 546 | item slots, skill slots, buffs, menu icons; icon texture is set by **code**, not the panel |
| Static | 338 | labels, captions, list rows |
| Image | 199 | backgrounds, frames, bars, lamps, portraits |
| Button | 190 | tabs, dialogs, menus |
| GaugeBar | 17 | HP/MP/FP/EXP |
| ListView | 11 | server list, chat, quest, party, guild, help |
| ImeEditBox | 6 | login, chat, guild |
| CheckBox | 4 | party auto-join, trade, guild |
| EditBox | 2 | login |
| ImeNotePad | 2 | guild notice |

**11 of the 21 registered element types never appear in shipped data** (Slider,
ComboBox, TreeView, RadioButton, ImageFont, MoMesh, CustomListView, BalloonBox,
TooltipBox, `ImeNotePadEditBox`, and the unused `EditBox`/`ImeEditBox` variants
are mostly aliases anyway). Do not build them until a panel needs one.

### 2.2 Visual feature usage

| Feature | Count | Effort implication |
|---|---:|---|
| `RasterOp split` | 97 | 9-slice is required |
| `RasterOp stretch` | 47 | trivial |
| `RasterOp wrap` | 6 | tiling |
| `RasterOp wSplitStretch` | 1 | horizontal split + vertical stretch |
| `RasterOp hStretchSplit` | 0 | can defer |
| `MultiLine true` | 13 | wrapping needed |
| `RichText true` | 12 | can defer behind a tag-strip flag in v1 |
| `UseAni true` | 1 | sheet animation is almost unused |
| `Password true` | 1 | one login field |
| Panel `Animation` | 19 | **all 19 use the identical Open/Close fade JSON** |

The 19 animation values are all this script:

```json
[{"Name":"Open","ScriptOne":[{"Time":0,"Type":3,"Target":"Panel",
  "nParam0":100,"fParam0":0,"fParam1":1}]},
 {"Name":"Close","ScriptOne":[{"Time":0,"Type":3,"Target":"Panel",
  "nParam0":100,"fParam0":1,"fParam1":0},
  {"Time":100,"Type":1,"Target":"Panel","nParam0":0}]}]
```

Implementing `Type 3` (alpha tween) and `Type 1` (hide/close after 100 ms)
covers every shipped panel. A general animation engine is unnecessary.

### 2.3 Texture aliases and atlases

- `UI/Default.guimat` is plain UTF-16 text: 21 fonts, 49 textures, 303 aliases.
- 138 distinct non-empty aliases are used by panels; **all 138 resolve**.
- 7 texture-list rows point at files that are not in the distribution
  (`NUI/ItemRandomBox.wyt`, `NUI/menu.wyt`, `NUI/gp_*`, `NUI/Settings2.wyt`),
  but **no shipped panel uses an alias that maps to them**. They belong to the
  game-settings panel which is not in the shipped panel set.
- 36 of the 49 textures are used by panel aliases, and **all 36 exist**;
  together they are ~24 MB. The largest are `ui/Image/NewUI_ui0.wyt` (4 MB),
  `UI/itemicon10.wyt` (2.9 MB), `NUI/Collection.wyt` (1.25 MB), and the
  encrypted PNG `ui/Image/NewUI_guimat.wyp` (1 MB).

Formats:

| Extension | Current TMProject support | Needed change |
|---|---|---|
| `.wyt` | yes (`TextureManager`, strips `WT10`, appends TGA footer) | reuse; factor a path-based loader |
| `.wys` | yes (`TextureManager`, repairs `WS10` → DDS header) | reuse |
| `.wyp` | **no** | read file → delta-decrypt → PNG bytes → `D3DXCreateTextureFromFileInMemoryEx` |

The delta cipher is documented in
[`../wyd-pane/01-container-and-crypto.md`](../wyd-pane/01-container-and-crypto.md);
the Python reference implementation exists in `docs/wyd-pane/tools/pane_codec.py`.

### 2.4 Fonts

`FontList` has 21 entries, sizes 10–44, two face indices. The distribution ships
`NanumBarunGothic.ttf` (4.2 MB) and `FontNanum.ttf` (2.3 MB). Two ways to match
the 2026 look:

1. **D3DXFont / GDI with the TTF** (recommended): `D3DXCreateFontW` is already
   linked, and `RenderDevice` already has `m_pDXFont`/`m_pDXFontLarge` hooks for
   `OnLostDevice`/`OnResetDevice`. One font object per guimat entry, cached per
   face+size. Handles UTF-16, wrapping and alignment.
2. Reuse `TMFont2` (GDI per-string upload, ANSI, 120-char cap). Cheaper to
   start but cannot render UTF-16, multiple sizes or wrapping well.

The 2026 binary contains references to `CreateFontW`, `TextOutW`,
`DrawTextExW`, `D3DXCreateFontW` and `D3DXCreateFontIndirectW`, which is
consistent with option 1. Plan for a new `GuiFont` wrapper; leave `TMFont2`
untouched for the legacy UI.

### 2.5 Localization

- Panel values like `Caption uu_210` are keys into the UI string table.
- **212 distinct `uu_*` keys** are used, and **all exist** in
  `Lang/EN/UIString.txt` (343 entries).
- The 748 table (`UI/UIString.txt`, 228 parsed entries) and the 2026 table share
  228 indices but **47 differ** (`14 Esc→Exit`, `24 Guild→Citizen`,
  `58 Quest→Quiz`, …). Do **not** overwrite the 748 file.
- Load `Lang/EN/UIString.txt` as a second table for pane text. `uu_N` maps to
  line N. Values are underscore-separated because the 748 loader used
  `sscanf("%d %s")`; `BASE_UnderBarToSpace()` already exists for display.
- 3 lines contain spaces (`238`, `239`, `320`) and the old loader would truncate
  them; read the rest of the line instead of `%s`.

This makes localization one of the cheapest parts of the project (~0.5 pw), not
a content project.

## 3. Panel → 748 feature map

The 748 code already implements most of the corresponding features
(`TMFieldScene.cpp` contains AutoTrade, Guild, Mix4, PGT, Portal, Quest,
SkillMaster, RPS, help, option stone, …). The port is mostly **re-expressing an
existing screen as data + a named-panel controller**, not inventing gameplay.

### Wave 1 — login and chrome (~150 elements, 21 files)

This is the closest match to 2026: the shipped `SelServerScene3.bin` contains
only 2 background panels; the entire login/server UI is panes. The 748
`TMSelectServerScene` can keep its background and 3D demo, but its code-built
login panel is replaced.

| Panels | 748 equivalent | Notes |
|---|---|---|
| `login/GameLogin`, `GameLogin2`, `GameLogin3`, `GameLoginItem`, `GameLoginItem2`, `logo` | code-built login in `TMSelectServerScene` (`m_pLoginPanel`, `m_pEditID`, `m_pEditPW`, `m_pLoginBtns[3]`) | 2026 loads GameLogin/2/3 from `0x51a700`; two ListViews (servers), edit boxes, logo |
| `game/ServerSelectPopup` | `m_pNServerSelect` / `m_pNServerGroupList` / `m_pNServerList` | loaded by `0x5e9930`; list + button |
| `login/CharSelect_Menu`, `_Create`, `_Password`, `_SelInfo` | `TMSelectCharScene` | 2026 **mixed**: `SelCharScene2.bin` (107 controls) plus 4 panes loaded from `0x512c50`; keep the 3D preview |
| `msgbox`, `msgbox_coininput`, `msgbox_editbox`, `msgbox_progress`, `msgbox_sysLoading` | `SMessageBox` / `SMessagePanel` | replaces runtime-built message boxes |
| `notice` | message panel / notice | 2 elements |
| `Tooltips/ToolTip`, `ToolTip_Item` | `m_pDescPanel` / item tooltip | tooltip timing is code |
| `game/SystemMenu` | field system menu | 7 elements |

### Wave 2 — field HUD, chat, help (~263 elements, 14 files)

The field scene is the key difference from the naive "panes replaced the UI"
assumption: 2026 still loads `FieldScene2.bin` with **1,354 controls**. The pane
HUD is a second layer. For our port, keep the working 748 field `.bin`
(709 controls), add the pane HUD/chat, and explicitly hide/disable the classic
controls that the panes replace. That suppression is a real integration task,
not a data conversion.

| Panels | 748 equivalent | Notes |
|---|---|---|
| `game/HUD` | skill belt, buffs, EXP, menu buttons, CCMode | 83 elements; 2026 loads HUD/HudMyInfo/HudYouInfo from `0x5ce5a0` |
| `HudMyInfo`, `HudYouInfo` | own/target HP/MP/FP + buffs | gauges + 32 buff slots each |
| `PartyInfo`, `PartyInfoItem` | party window + row | ListView + itemPanel |
| `MiniPopup`, `PortalList`, `PortalListItem` | small popups / portal list | 2026 loads FieldScene + MiniPopup from `0x5c17c0` |
| `chat`, `chat2`, `chatItem`, `ChatMenu` | chat edit box, chat list, menu | IME edit box + ListView rows |
| `Helpbook`, `HelpbookItem` | help window | ListView |

**Not covered by any pane** (stays legacy): minimap, world-space name labels,
damage numbers, item drop labels, cursor, and the ground/character rendering.
Plan a "pane HUD + legacy overlays" composite, not a total replacement.

**Not replaced unless you choose W3:** the 748 inventory/cargo/shop/trade grids.
2026 keeps 185 `GRID` controls in `FieldScene2.bin` *and* ships
`PanelInventory`/`Inventory.pane`; the two branches coexist. Keeping the 748
grids is the cheapest way to get a modern HUD without rewriting item logic.

### Wave 3 — item windows (~687 elements, 12 files)

| Panels | 748 equivalent | Notes |
|---|---|---|
| `Inventory` (88), `Character_Base` (179), `Character_Skill` (56) | inventory/equipment grids + character window | 78 IconButtons in inventory; equipment slots named `Equip1..17` |
| `ItemCargo` (127) | storage cargo grid | 120 IconButtons |
| `ItemShop` (43), `ItemTrade` (39), `AutoTrade` (19) | shop, trade, auto-trade | |
| `ItemMix` (44), `ItemMix4` (10), `ItemRefine` (36), `ItemOptionStone` (16) | mix / mix4 / refine / option stone | |
| `SkillMaster` (30) | skill master window | 24 IconButtons |

This is where the real work is: the pane is only the layout. The item model,
drag/drop, validation, packet sends and tooltips live in `SGridControl`
(5,332 lines) and `TMFieldScene` today. Because 2026 keeps both the pane
inventory and 185 classic grids, **W3 is optional for a first release**: keep
the 748 grids and adopt panes for login/HUD/chat (see the scope levers in
`03-plan-and-effort.md`).

### Wave 4 — panels for thin/absent 748 features (~209 elements, 16 files)

`Quest`, `QuestTitleItem`, `Collection_Base`, `Collection_Item`,
`Guild_Base/Create/Info/Member/MemberItem/MemberPopup/NoticeEdit`, `Arena`,
`RpsGame`, `PGTMenu`, `GameConfig`, `ItemOptionStone.panel` copy.

748 has code for Quest, Guild, PGT and RPS; `Collection`, `Arena`,
`ItemRefine` and `GameConfig` are thin or absent. Load and render these panels
only when the matching server opcode exists; otherwise keep them behind a flag.
`GameConfig` itself is renderable from shipped data (its aliases use
`NUI/GameConfig.wyt`); the 7 missing texture rows are unused by every panel.

### Legacy-only screens with no 2026 pane

`FireWork`, `Toto`, `Gamble`, `Quiz`, `Mission`, the guild-mark editor, and the
minimap. They keep the 748 look unless new panes are authored. Decide whether
that inconsistency is acceptable before S6.

## 4. Asset staging (do not overwrite 748 files)

Many 2026 UI files have the same names as 748 files but different contents
(`character2.wyt`, `cursor.wyt`, `CommonBox.wyt`, `hppro.wyt`, …). Copying the
2026 `UI/` tree into `client_748/UI/` would break the existing 748 screens.

Recommended layout:

```text
client_748/
  UI/            existing 748 assets (untouched)
  NUI/           existing 748 assets (untouched)
  ui2026/
    UI/Panel/    the 63 panes
    UI/          only the 49 guimat textures that are referenced
    NUI/         referenced NUI textures
    UI/Default.guimat
    UI/balloon.guimat
    Lang/EN/UIString.txt
    UI/FontNanum.ttf, UI/NanumBarunGothic.ttf
```

The pane loader resolves `Resource`, `TextureList` paths and `itemPanel` paths
through a configurable root (default `ui2026/`). A staging script copies only
what is referenced (≈31 MB) and writes an asset manifest so the build can fail
if a texture is missing.

Do not route pane textures through `TextureManager`'s 512 UI slots; use a
dedicated `GuiTextureCache` keyed by path, with explicit device-loss handling.
That keeps the 748 `UITextureList` indices untouched.

## 5. What is already in our favour

1. The format, crypto, grammar and widget behaviour are documented with
   addresses and pseudocode.
2. Python codec/parser/schema tools already round-trip every panel.
3. All 63 panels parse; all used aliases and textures exist.
4. The widget surface is 10 types, not 21.
5. Element names are semantic; binding does not need a numeric ID table.
6. `.wyt`/`.wys` loading already exists; only `.wyp` is new.
7. `D3DXCreateFontW` is already linked and `m_pDXFont` hooks exist.
8. `uu_*` localization maps 1:1 to an existing string-table concept; the 2026 EN
   table covers all 212 keys.
9. The 2026 client is a verified hybrid: it still instantiates all three scene
   classes and loads `FieldScene2.bin`/`SelCharScene2.bin`, so `SControl` and the
   field scene can stay.
10. The login/server-select UI is pane-only in 2026 (`SelServerScene3.bin` is
    just 2 background panels), which makes it an ideal pilot with no classic-UI
    conflict.
11. The renderer already has `RenderRectTex` (src rect + dest size + tint +
    rotation + alpha), which covers most pane drawing.

## 6. What is against us

1. **9-slice margins and the exact panel scale policy are not documented.** Both
   must be recovered from `FUN_00477820` / measured from screenshots.
2. **Classic-control suppression.** The 2026 field scene still initializes its
   1,354 `.bin` controls; when panes replace a window, the corresponding classic
   controls must be hidden or bypassed or both layers will draw/click at once.
   The 2026 selector and its suppression logic are not yet identified (U17).
3. **Two input stacks.** The 748 container walks the whole tree and has eight
   modal slots; panes need capture, focus and a modal stack. They must be
   arbitrated, or clicks will pass through.
4. **Item gameplay is not in the panes.** `SGridControl` owns drag/drop, stack
   splitting, buy/sell, tooltips and many packet sends.
5. **Text quality.** The current pipeline is ANSI, GDI-per-change, 120 chars,
   three chunks. Pane text is UTF-16, multi-size, aligned, sometimes multiline.
6. **Draw order.** Legacy is depth-first with a 30-layer cap and prepended
   children; panes use `ZOrder` and `BelowPanel`. A compositor is required.
7. **Asset collisions.** Same filenames, different art. Must be isolated.
8. **TMFieldScene.cpp is not UTF-8** (13 raw bytes on 9 lines) and is 24k lines;
   new code should live in new files and touch it only at existing hooks.
9. **Testing needs a server.** The docs already note the field is untested
   in-game because no server was running. Without a server or a packet-replay
   harness, S4–S5 cannot be verified. Budget for a local test server or replay.
10. **Legacy screens with no pane** (firework/toto/gamble/quiz/mission) will look
    inconsistent.
11. **REA cannot open the `E:\` copy** (path admission error). Work from
    `C:\Users\Luis\AppData\Local\Temp\rea-wyd\WYD.exe` (same SHA-256) as done
    for this review.

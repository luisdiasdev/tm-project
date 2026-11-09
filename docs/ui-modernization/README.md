# UI Modernization — Porting the 2026 `.pane` UI to TMProject

Status: **plan / estimate**, not started. Branch `test/748`. Written 2026-11-08.

This folder is the effort plan for using the newer `.pane` UI system documented
in [`../wyd-pane/`](../wyd-pane/README.md) in the 748 client. It assumes:

- the game logic, packets and server stay **748**;
- the 2026 UI assets (`../wydglobal2026`) may be shipped alongside them;
- we keep the D3D9 renderer and `SControl` (we do not rewrite the field scene).

Read this file first. Detail is in the numbered documents.

## TL;DR

**Feasible, and cheaper than the raw panel count suggests — but it is a real
widget-toolkit port, not a data migration.**

- The format is fully documented and the shipped data is consistent:
  63 distinct panels, **1,315 element records**, **10 of 21** widget types
  actually used, **138 distinct texture aliases**, and **every alias used by a
  panel resolves to a texture that ships**.
- The 2026 client is a **verified layered hybrid**, not a rewrite. Its scene
  factory still instantiates `TMFieldScene`/`TMSelectCharScene`/
  `TMSelectServerScene`, and `TMFieldScene::InitializeScene` loads
  `UI\FieldScene2.txt` → `FieldScene2.bin` with **1,354 classic controls**
  (`SelCharScene2.bin` has 107). Panes are added on top. Login/server select is
  pane-only (`SelServerScene3.bin` is 2 background panels; there is no
  `LoginScene` string). **We should copy that hybrid design.**
- The binding layer is better than the 748 one: panel elements have semantic
  names (`Inven0`, `Equip1`, `Skill0`, `TabItem0`, `ChatEditBox`, …). No new
  magic IDs.
- Most cost is not parsing. It is **text/IME, input arbitration between two UI
  stacks, and the item windows**, whose behaviour lives in `SGridControl`
  (5,332 lines), not in the panels.

### Effort at a glance

Developer-weeks (1 pw = 5 focused days) for one engineer who knows this
codebase. Excludes art direction, server features, and product decisions. For
calendar projections (solo vs AI-assisted), see
[`03-plan-and-effort.md`](03-plan-and-effort.md#8-calendar) — the low bound
barely moves because it is debugging/parity time, not typing.

| Scope | Effort | What you get |
|---|---:|---|
| **S1 — hello pane** | **10–16 pw** | `notice.pane` / `msgbox.pane` rendered in-game, correct crypto, guimat, textures, text |
| **S2 — interactive widgets** | **24–38 pw** | Button/CheckBox/IconButton/GaugeBar/EditBox/ListView, focus, hover, click, modal |
| **S3 — login & chrome** | **28–44 pw** | Login, server popup, character select, message boxes, notice, tooltips, system menu on `.pane` |
| **S4 — field HUD & chat** | **31–50 pw** | Modern HUD, HP/MP/FP, buffs, party, chat list/edit box, help — playable field; 748 item grids stay |
| **S5 — item windows** | **37–60 pw** | Inventory, equipment, cargo, shop, trade, auto-trade, mix, refine, option stone |
| **S6 — all panels + parity** | **45–71 pw** | Quest, guild, collection, arena, RPS, PGT, config; animations, resolution matrix, hot reload |

Recommended commitment: **fund S1 (10–16 pw) as a proof of the pipeline**, then
S2 (24–38 pw cumulative) as the actual runtime investment with a hard go/no-go
before any screen migration.

**First release: S4 (31–50 pw).** It delivers pane login, chrome, HUD and chat
while keeping the proven 748 item grids — a combination the 2026 client itself
still ships (185 classic grids alongside `Inventory.pane`). S5 upgrades the
item windows; S6 is optional and mostly panels for 748-thin or 748-absent
features.

## The one decision that dominates the cost

There are three ways to put `.pane` on screen. Pick one before P1:

| Option | Effort to first usable UI | Fidelity | Verdict |
|---|---:|---|---|
| **A. Native `CGui*` runtime** (recommended) | 10–16 pw to first pane; 24–38 pw for the full interactive engine | 1:1 with 2026 | Clean, supports animation/state slots/9-slice, future-proof |
| B. Translate `.pane` into `SControl` widgets | 10–14 pw | ~70% | Fast, but the widget model (state slots, 9-slice, anchors, per-font text) does not fit; becomes dead-end debt |
| C. Reskin the 748 UI only (no format) | 3–5 pw | ~40% of the look | Cheapest; keeps 748 layout and text pipeline; no 2026 windows |

Option A is recommended because the 2026 panes are data, and the point of the
exercise is to stop hand-writing UI in C++. Option B still leaves every new
window as C++ work. Option C is a valid "make it prettier" project but does not
deliver the 2026 UI.

## Facts this plan is based on

Verified in this tree; details and reproduction in
[`01-scope-and-assets.md`](01-scope-and-assets.md).

| Fact | Value |
|---|---|
| 2026 scene system | **live**: `FieldScene2.bin` 1,354 controls, `SelCharScene2.bin` 107, `SelServerScene3.bin` 2 background panels; the scene factory instantiates all three classes |
| 2026 pane system | **live**: `PanelHUD`/`PanelInventory`/`PanelFieldScene`/login/char-select controllers load their `.pane` paths through `FUN_00482c10` |
| Panels | 63 distinct (64 files; `ItemOptionStone` has a `.pane` and a plaintext `.panel` copy) |
| Element records | 1,315 |
| Types used | IconButton 546, Static 338, Image 199, Button 190, GaugeBar 17, ListView 11, ImeEditBox 6, CheckBox 4, EditBox 2, ImeNotePad 2 |
| Types never used | Slider, ComboBox, TreeView, RadioButton, ImageFont, MoMesh, CustomListView, BalloonBox, TooltipBox, ImeNotePadEditBox |
| Draw modes used | `split` 97, `stretch` 47, `wrap` 6, `wSplitStretch` 1 (no `hStretchSplit`) |
| Rich features | `RichText` 12, `MultiLine` 13, `UseAni` 1, `Password` 1, panel `Animation` 19 (all the same Open/Close fade script) |
| Texture aliases | 303 defined, 138 used, **0 missing** |
| Textures | 49 rows; **36 used by panels (~24 MB), all exist**; 7 unused rows point at files not shipped |
| Fonts | 21 guimat font entries (sizes 10–44), 2 TTFs, ~6.2 MB |
| Strings | 212 `uu_*` keys, all present in `Lang/EN/UIString.txt` (343 entries) |
| Data size | panels 280 KB; textures ~24 MB used (~25 MB shipped); fonts ~6.2 MB |

## What `.pane` fixes, and what it does not

| Problem today | Fixed by `.pane`? |
|---|---|
| Layout/style fused into C++ | Yes — layout, textures, colors, fonts become data |
| Numeric control IDs / `FindControl` | Yes for new panels — bind by element name |
| Silent missing texture / control | Yes if we implement the validator (fail loud) |
| Multi-resolution drift | Partly — anchors exist, but the exact scale policy is undocumented (see risks) |
| Rebuild-and-relogin iteration | Improved by data files; hot reload is a separate item (W5) |
| Item drag/drop, buy/sell, packets | **No** — still `SGridControl`/`TMFieldScene` code |
| Full replacement of the field UI | **No** — 2026 still loads the classic `.bin` scene; panes are additive |
| Minimap, world labels, damage text, legacy popups | **No** — not part of the pane set |

## Milestones and gates

| Gate | After | Pass condition |
|---|---|---|
| **G0** | P0 | Reference captures exist for every wave-1 panel at 800×600 and 1024×768; 9-slice/scale/font policies written down |
| **G1** | S1 | All 63 panes parse with 0 unknown keys; `notice.pane` renders in-game within ±2 px of the 2026 capture |
| **G2** | S2 | Widget gallery passes interaction checklist (hover/press/disabled/focus/wheel/Enter/Esc) |
| **G3** | S3 | Login → server select → character select runs entirely on `.pane` chrome; legacy input never steals a pane click |
| **G4** | S4 | Field runs with modern HUD/chat; no `FindControl` ID changed; legacy popups still work |
| **G5** | S5 | Inventory/cargo/shop/trade item flows pass the 748 regression checklist |
| **G6** | S6 | Resolution matrix, fade animations, tooltips, IME and hot reload verified |

A gate is a stop. The cheapest place to learn that 9-slice or scaling is wrong
is G1, not after migrating the HUD.

## First four weeks (concrete)

1. **Use the working REA copy.** REA rejects the `E:\` tree; work from
   `C:\Users\Luis\AppData\Local\Temp\rea-wyd\WYD.exe` (same SHA-256). Decompile
   `FUN_00477820` (the quad/9-slice renderer), `FUN_00478ec0` (panel draw),
   `FUN_0047a110` (guimat), `CGuiIconButton` draw, and the `Panel*` init methods.
   Note: if your disassembler shows `0x676042`/`0xA1BA6C`, it is rebased by
   `+0x190000` from the file's `0x400000`; the file addresses are
   `0x4E6042`/`0x88BA6C`.
2. **Capture the reference.** Run the 2026 client at 800×600 and 1024×768 with
   `docs/tools/capture_and_click.ps1`; save one PNG per panel and measure the
   HUD/chat width to settle the scale policy.
3. **Build the Python previewer** (extend `docs/wyd-pane/tools/`): decoded JSON
   + `texture_extract.py` → PNG per panel at 800×600. This is the golden
   reference for the C++ renderer and for codegen tests.
4. **Write the decisions** in [`04-risks-and-decisions.md`](04-risks-and-decisions.md):
   native vs adapter, scale policy, 9-slice margins, font path, item-window
   strategy.
5. **Start P1** (parser + validator) in parallel; it does not depend on the
   policy answers.

## Reading order

1. [`01-scope-and-assets.md`](01-scope-and-assets.md) — what the 2026 client
   actually does, what data ships, and how the panels map onto 748 features.
2. [`02-architecture.md`](02-architecture.md) — target layering, modules,
   integration points, binding/input/draw models.
3. [`03-plan-and-effort.md`](03-plan-and-effort.md) — work packages, estimates,
   milestones, scope levers, test strategy.
4. [`04-risks-and-decisions.md`](04-risks-and-decisions.md) — unknowns, risks,
   decision log, go/no-go.

## Assumptions

- Primary target is **800×600** (the pane coordinates are in that space), with
  1024×768 as the first scaling test.
- English only (`Lang/EN`); the 2026 EN string table is loaded **beside** the
  748 `UI/UIString.txt`, not over it (47 of 228 common indices differ).
- The 748 server is the source of truth for gameplay; 2026-only features are
  out of scope until the server supports them.
- The `.bin` scene stays. Panes are additive; a pane that replaces a classic
  window must hide the classic controls (U17).
- 2026 UI assets are staged in a separate folder (e.g. `client_748/ui2026/`)
  because many filenames collide with 748 textures and must not overwrite them.
- No third-party UI framework. D3D9, GDI/D3DX fonts and the existing texture
  loaders stay.
- No visual editor in this plan. The Python previewer + validator are the
  authoring aids; an editor can be added later against a stable schema.

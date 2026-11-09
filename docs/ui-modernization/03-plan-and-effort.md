# 03 — Plan, work packages and effort

## 1. Estimation method

- Unit: **developer-week (pw)** = 5 focused days for one engineer who already
  knows this codebase.
- Ranges are uncertainty, not padding. Low = nothing surprising; high = the
  usual decompiled-code archaeology plus parity iteration.
- Excludes: art direction, new server features, packet work, product decisions,
  and QA outside the stated gates.
- The 748 tree already provides the D3D9 device, texture decoding, input
  plumbing and a text path. Those are assumed to work; they are not re-estimated.
- AI assistance mainly compresses boilerplate (widgets, parsers, tools, screens).
  It does not compress debugging an input/focus conflict or matching pixels.

## 2. Work breakdown

### P0 — Spike, references, decisions (1.5–3 pw)

| Task | Effort |
|---|---:|
| Use REA from `C:\Users\Luis\AppData\Local\Temp\rea-wyd` (the `E:\` copy is rejected); decompile `FUN_00477820`, `FUN_00478ec0`, `FUN_0047a110`, `FUN_00466bb0`, `CGuiIconButton` draw | 0.5–1 |
| Run the 2026 client at 800×600 and 1024×768; capture every wave-1 panel; measure scale and split behaviour | 0.5–1 |
| Python previewer v0: decoded JSON + extracted textures → PNG per panel | 0.5–1 |
| Document the classic/pane split and the suppression rule (U17): which `SControl` controls each pane replaces, and how 2026 hides them | 0.25–0.5 |
| Optional C++ smoke test: one `.wyt` through `D3DXCreateTextureFromFileInMemoryEx`, one tinted `RenderRectTex` quad, one D3DXFont string | 0.25–0.5 |
| Write decisions D1–D8 (native vs adapter, scale, 9-slice, font, item strategy) | included |

Exit: reference PNGs and a written decision record. **No C++ yet.**

### P1 — Pane IO, model, guimat, assets (3–5 pw)

| Task | Effort |
|---|---:|
| Delta decrypt/encrypt, UTF-16 BOM read, line tokenizer, block parser | 0.75–1.25 |
| Ordered attribute model, `Panel` + element blocks, comments, re-emit | 0.75–1 |
| Typed schema (panel/element/per-type keys), enum/default handling, validator with unknown-key report | 0.75–1.25 |
| `Guimat`: FontList/TextureList/TextureAlias parse + case policy + alias cache | 0.5–0.75 |
| `GuiTextureCache`: `.wyt`/`.wys` reuse, `.wyp` decrypt→PNG, color key, lazy load, device loss | 0.75–1 |
| Manifest/codegen: element names, asset list, hashes, duplicate-name report | 0.25–0.5 |
| CLI validator wired into the build (all 63 panes) | 0.25 |

Exit: all 63 panes parse with 0 unknown keys; every used alias resolves; the
manifest reproduces and validates.

### P2 — Renderer and text (3–5 pw)

| Task | Effort |
|---|---:|
| `GuiRenderer::DrawQuad` over `RenderRectTex`; stretch/wrap/rotation | 0.5–1 |
| `split` + `wSplitStretch` 9-slice decomposition with configurable margins | 0.75–1.25 |
| Scissor clipping (list viewports, edit boxes) + state restore | 0.25–0.5 |
| `GuiFont`: guimat index → face/size, D3DXFont (or GDI) UTF-16 draw, caching, device loss | 0.75–1.25 |
| Alignment, multiline/wrap, shadow modes, `FontOffset`, `uu_*` resolution | 0.5–0.75 |
| IME composition draw/candidate placement (can slip to S2) | 0.25–0.75 |

Exit: `notice.pane` and `msgbox.pane` render at 800×600 within ±2 px of the
reference; text is legible at all guimat sizes.

### P3 — Widgets actually used (11–16 pw)

Only the 10 types present in shipped data. Slider/ComboBox/TreeView/RadioButton/
ImageFont/MoMesh/CustomListView are **not built**.

| Widget | Effort | Notes |
|---|---:|---|
| `Static` | 1–1.5 | caption/text, multiline, alignment, shadow; rich text behind a flag |
| `Image` | 0.75–1.25 | slot 0, stretch/wrap/split, external file, rotation, sheet animation |
| `Button` + `CheckBox` + `IconButton` | 2.5–4 | 4 state slots, hover/press/disabled, groups/tabs, toggle, ok/cancel, sounds, tooltip hook |
| `GaugeBar` | 1.25–1.75 | range/pos, 4 directions, outspace, text fraction/percent, flash/spark |
| `EditBox` / `ImeEditBox` | 2.5–3.5 | caret, selection, clipboard, password, range, IME, focus |
| `ListView` | 2.5–4 | item vector, scrollbar, selection, wheel/keys, `itemPanel` row instantiation, grid arrangement |
| `ImeNotePad` | 0.5–0.75 | guild notice; can be deferred |

Exit: a widget gallery panel plus the real message boxes and system menu pass
the interaction checklist.

### P4 — Panel runtime, animation, input (3–4.5 pw)

| Task | Effort |
|---|---:|
| `GuiPanel`: attributes, anchors (`NONE`/`Center`/`SIDE`), offsets, screen align, `ZOrder`, titlebar, caption, close button, page index, sounds | 1–1.5 |
| Panel Open/Close fade (`Type 3`, `Type 1`) and image sheet tick | 0.5–0.75 |
| `GuiManager`: registry, open/close, exclusive windows, modal stack, sorted draw | 0.5–0.75 |
| Input router: hit test, capture, focus hand-off, wheel, Enter/Esc, legacy arbitration | 1–1.5 |

Exit: modal message box blocks the field; Enter/Esc work; clicks never pass
through a pane to a legacy control or vice versa.

### P5 — TMProject integration (2.5–4 pw)

| Task | Effort |
|---|---:|
| Scene hooks: begin/end, tick, draw after legacy layers, cleanup, window resize | 0.75–1 |
| **Classic-control suppression**: hide/disable the `SControl` windows replaced by panes (login, HUD, chat) without breaking their game logic | 0.75–1.25 |
| Dual input/focus arbitration with `SControlContainer` (incl. IME ownership) | 0.75–1.25 |
| Legacy `GuiLegacyHost` for `SGridControl` subtrees (bridge only) | 0.5–1 |
| Pane string table (`Lang/EN/UIString.txt`) + `uu_*` + `_`→space | 0.25–0.5 |
| Asset staging (`ui2026/`), manifest, startup validation, log output | 0.25–0.5 |

Exit: a pane HUD/msgbox runs in the field scene with all legacy screens intact.

### W1 — Login and chrome (4–6 pw)

| Screen | Effort |
|---|---:|
| `GameLogin` + 2 ListViews + edit boxes + logo (server list/connect) | 1.5–2 |
| `CharSelect_Menu` / `_Create` / `_Password` / `_SelInfo` (keep 3D preview) | 1.5–2.5 |
| `msgbox` ×5, `notice`, tooltips, sys loading | 0.75–1.25 |
| `SystemMenu` + help entry points | 0.25–0.5 |

Exit: login → server → character select works on pane chrome. In 2026 the
server-select `.bin` is only 2 background panels, so this is the cleanest
screen group; char select is mixed and keeps its `.bin` scene.

### W2 — Field HUD, chat, help (3.5–5.5 pw)

| Screen | Effort |
|---|---:|
| `HUD` (skills/buffs/quickslots/EXP/menu/CCMode) | 1.5–2.5 |
| `HudMyInfo`, `HudYouInfo`, `PartyInfo`, `PartyInfoItem` | 1–1.5 |
| `chat`, `chat2`, `chatItem`, `ChatMenu` | 0.75–1.25 |
| `Helpbook`, `HelpbookItem`, `MiniPopup`, `PortalList` | 0.25–0.5 |

Exit: playable field with modern HUD and chat; legacy overlays still render.
The classic HUD controls in `FieldScene2.bin` are suppressed, not removed, so a
flag can restore them.

### W3 — Item windows (6–10 pw)

This is the risky wave, and it is **optional for a first release**: 2026 keeps
the 185 classic grids and `SGridControl`-style logic alongside the pane
inventory. Each window has two parts: panel layout (cheap) and item behaviour
(expensive). Start with the simplest and reuse one `GuiItemGrid` controller
across windows.

| Screen | Effort | Behaviour source |
|---|---:|---|
| `Inventory` + equipment slots + tabs + drag/drop | 2–3.5 | `SGridControl`, `TMFieldScene` |
| `ItemCargo` | 1–1.75 | storage grid |
| `ItemShop`, `ItemTrade`, `AutoTrade` | 1.5–3 | shop/trade grids |
| `ItemMix`, `ItemMix4`, `ItemRefine`, `ItemOptionStone` | 0.75–1 | `MrItemMix`, refine/option handlers |
| `Character_Base`, `Character_Skill`, `SkillMaster` | 0.75–0.75 | character/skill windows |

Exit: 748 item flows pass the regression checklist; no packet changes.

### W4 — Thin/absent features and extra panels (4–6 pw, optional)

Quest, QuestTitleItem, Collection, Guild (7 panels), Arena, RPS, PGT, GameConfig.
Most map to some 748 code (Quest/Guild/PGT/RPS); Collection/Arena/Refine/
GameConfig are thin or absent. Load/render only when the server path exists.
`GameConfig` is renderable from shipped data (its aliases resolve to
`NUI/GameConfig.wyt`); it is gated by feature support, not by assets.

### W5 — Parity, tooling, polish (3–5 pw)

| Task | Effort |
|---|---:|
| Resolution matrix (800×600, 1024×768, 1280×1024) and anchoring fixes | 1–1.5 |
| Animation/fade polish, tooltip timing, sounds, cursor changes | 0.75–1 |
| Interaction regression suite (click/scroll/edit/IME/modal) | 0.5–1 |
| Hot reload (watch `ui2026/`, rebuild one panel, keep state) | 0.5–1 |
| Documentation and validator in CI | 0.25–0.5 |

## 3. Roll-up

| Block | Effort (pw) |
|---|---:|
| P0 spike | 1.5–3 |
| P1 pane IO/assets | 3–5 |
| P2 renderer/text | 3–5 |
| P3 widgets | 11–16 |
| P4 panel runtime/input | 3–4.5 |
| P5 integration | 2.5–4 |
| **Runtime subtotal** | **24–38** |
| W1 login/chrome | 4–6 |
| W2 HUD/chat/help | 3.5–5.5 |
| W3 item windows | 6–10 |
| W4 thin/absent features | 4–6 |
| W5 parity/tooling | 3–5 |
| **Screens subtotal** | **20.5–32.5** |
| **Total** | **45–71** |

The runtime subtotal is the number that matters for the go/no-go: if the team
will not spend ~24–38 pw on a widget toolkit, Option B (adapter) or C (reskin)
is the honest choice.

Ranges are ±20%. The classic/pane suppression work (U17) sits inside P5's high
end; the P0 split investigation sits inside P0's high end. Neither changes the
rounded totals.

## 4. Milestones

Cumulative effort. Each milestone ships something demonstrable.

| Milestone | Content | Cumulative | Demo |
|---|---|---:|---|
| **S1 — hello pane** | P0, P1, P2, plus `Static`/`Image` and basic `GuiPanel` | 10–16 | `notice.pane` / `msgbox.pane` in-game |
| **S2 — interactive widgets** | rest of P2/P3, P4 | 24–38 | widget gallery + real message boxes/system menu |
| **S3 — login & chrome** | P5 + W1 | 28–44 | login → server → char select on panes |
| **S4 — field HUD** | + W2 | 31–50 | modern HUD/chat in a playable field; 748 item windows still active |
| **S5 — item windows** | + W3 | 37–60 | inventory/cargo/shop/trade/mix on panes |
| **S6 — full set** | + W4 + W5 | 45–71 | all 63 panels, resolution matrix, hot reload |

Recommended stop point for a first release: **S4**. It gives pane login,
chrome, HUD and chat while keeping the proven 748 item grids — a combination the
2026 client itself still ships (`FieldScene2.bin` has 185 grids alongside
`Inventory.pane`). S5 upgrades the item windows to the new pane look; S6 is
mostly panels for features where 748 is thin plus a full parity/tooling pass,
and can be scheduled later against a stable schema.

## 5. Scope levers

Use these to trade cost and fidelity before cutting phases.

| Lever | Saving | Cost |
|---|---:|---|
| Adapter (Option B) instead of native runtime | −10 to −20 pw | ~70% fidelity; no animation/state slots; dead end |
| Reskin 748 UI only (Option C) | −20 to −30 pw vs S1–S2 | no 2026 layout |
| Defer `ImeNotePad` + rich text + sheet animation | −1.5 to −2.5 pw | guild notice and a few item/quest texts look plain |
| Keep `SGridControl` for trade/shop/mix (host only) | −1.5 to −2.5 pw | legacy art inside new frames |
| **Keep the 748 item grids and stop at S4** (supported by 2026's own classic branch) | **−6 to −10 pw vs S5** | inventory/cargo/shop/trade keep 748 art and `SGridControl` logic |
| Skip item windows entirely and also drop the HUD (stop at S3) | −9 to −15 pw | only login/char/msgbox modern |
| Skip wave 4 | −4 to −6 pw | Collection/Arena/Quest/Guild panels unused |
| Drop 1024×768 scaling (800×600 only) | −1 to −1.5 pw | not resolution independent |
| Skip hot reload | −0.5 to −1 pw | slower iteration after delivery |
| Accept legacy-only screens | 0 pw | firework/toto/gamble/quiz/mission stay 748-styled |

## 6. Sequencing

```text
week:  1   2   3   4   5   6   7   8   9  10  11  12  13  14 ...
P0     ███
P1       ████████
P2         ████████
P3           ██████████████████
P4                     ████████
P5                       ████████
W1                          ██████████
W2                            ██████████
W3                                ██████████████████
W4                                      ██████████
W5                                        ████████
```

- P1 can start on day one; it has no dependency on the policy decisions.
- The Python previewer (P0) and W1 layouts can be prepared in parallel with P2/P3.
- P4's input router should have one owner; it is the main architectural risk.
- With two engineers, split engine (P1–P3) from screens/tooling, but keep the
  `GuiManager`/input design single-owner. Expect ~25–35% calendar compression,
  not 50%.

## 7. Test strategy

### 7.1 Automated, cheap, must exist before migration

| Test | What it proves |
|---|---|
| Codec round-trip (`encrypt(decrypt(x)) == x`) | container is correct |
| Parser round-trip (decode → model → re-emit, compare normalized) | no dropped/duplicated keys |
| Schema validation over all 63 panes | 0 unknown keys, 0 malformed values |
| Alias/asset manifest check | every used alias and texture resolves |
| Python previewer vs 2026 screenshot | layout, anchors, 9-slice, text metrics |

### 7.2 In-client gates

| Gate | Test |
|---|---|
| G1 | `notice`/`msgbox` screenshot diff at 800×600 |
| G2 | scripted hover/press/disabled/focus/wheel/Enter/Esc on the widget gallery |
| G3 | login flow: type ID/password, select server/channel, create/select/delete char |
| G4 | field: open/close every wave-2 panel, type in chat, scroll lists, use skills/buffs |
| G5 | item flows: pick/place/swap/split, buy/sell, trade, mix, refine, option stone |
| G6 | resolution matrix + Alt-Tab device loss + IME composition |

### 7.3 Regression guards

- All three `.bin` scenes still load byte-identically.
- Legacy `FindControl` IDs unchanged; `TMFieldScene.cpp` non-UTF-8 invariant
  preserved (13 raw bytes / 9 lines; use `docs/tools/restore_non_utf8.py`).
- A debug build parses all panes at startup and writes a one-line-per-panel
  report to `WYD.log` (elements, unknown keys, missing aliases, duplicate names).
- A "pane off" switch restores the 748 UI without a rebuild if a screen fails.

## 8. Calendar

Developer-weeks are effort, not calendar. At full-time focus:

| Scope | Effort | Calendar (AI-assisted, 0.75–1.0× midpoint) | Calendar (unassisted, 1.0–1.3×) |
|---|---:|---:|---:|
| S1–S2 spike | 24–38 | 5–7 months | 7–9 months |
| S3 (login/char) | 28–44 | 6–8 months | 8–11 months |
| S5 (recommended) | 37–60 | 8–11 months | 11–15 months |
| S6 (full) | 45–71 | 10–13 months | 13–17 months |

These are honest single-engineer numbers. They look large because this is a
widget toolkit plus 63 screens, not a reskin. If the goal is "prettier UI next
month", Option C is the right project; if the goal is "stop hand-coding UI in
C++ and inherit the 2026 layout", S5 is the right project.

## 9. Deliverables per milestone

| Milestone | Code | Data/tools | Docs |
|---|---|---|---|
| S1 | Pane IO, renderer, font, `Static`/`Image`, `GuiPanel` | validator, manifest, previewer | 9-slice/scale decisions |
| S2 | widget set, input router, modal stack | widget gallery panel | input/focus contract |
| S3 | P5 integration, `PanelLogin`, `PanelCharSelect`, `PanelMsgBox` | staged `ui2026/` + manifest | login/char port notes |
| S4 | `PanelHUD`, `PanelChat`, `PanelParty`, `PanelHelp` | HUD reference captures | overlay boundary list |
| S5 | `GuiItemGrid`, `PanelInventory`, `PanelCargo`, `PanelShop`, `PanelTrade`, `PanelMix` | item-flow test script | item behaviour mapping |
| S6 | remaining panels, hot reload | CI validator, screenshot diff | full schema and port guide |

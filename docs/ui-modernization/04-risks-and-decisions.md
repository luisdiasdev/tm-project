# 04 — Unknowns, risks, decisions and gates

## 1. Unknowns that must be closed before they cost real time

The format documents describe the data and the class model well. These are the
places where the answer is not in `docs/wyd-pane/` yet. Each has a cheap
resolution path and a fallback.

| # | Unknown | Why it matters | Resolution | Cost | Blocking? |
|---|---|---|---|---|---:|---|
| U1 | **9-slice margins** for `RasterOp split` (`FUN_00477820`) | 97 elements use split; wrong margins make every window frame visibly wrong | decompile `0x477820`; compare with 2026 captures; make margins configurable | 0.5–1 pw | Yes for fidelity, no for structure |
| U2 | **Panel scale policy** (`PanelScaleType`, `AlignPanelX/Y`, `OffsetX/Y`, `ScreenAlign`) | hit testing and layout break if panes scale differently from the legacy UI | measure HUD/chat width in 2026 at 800×600 and 1024×768; write the rule | 0.5–1 pw | Yes |
| U3 | **Font face mapping** (guimat `FontList` face 0/1 → which TTF) and exact rendering method | 21 font sizes drive every label | inspect `FUN_0047a110`/font loader; compare glyph metrics | 0.25–0.5 pw | No |
| U4 | **IconButton icon semantics** (`IconTexture`, `IconTextureSlot`, `IconSlotIndex`, `IconSlotType`) | 546 elements, but nearly all icons are set by code at runtime | decompile `0x42c550`; define a `SetIcon(atlas, slot, type)` API; map item/skill/buff data | 0.5–1 pw | No (runtime API is ours) |
| U5 | **ListView grid arrangement and itemPanel row binding** | 11 lists, and rows are how chat/quest/party/guild rows work | decompile `0x458840`/`0x4564e0`; prototype one list (server list) | 0.5–1 pw | No, but early |
| U6 | **RichText markup syntax** | 12 elements use it; item/quest text may need colors | decompile the text helper; v1 can strip tags | 0.5–1 pw | No (defer) |
| U7 | **Panel controller bindings and event ids** (`PanelHUD`, `PanelInventory`, …) | tells us which element names the 2026 code looks up and what it does on click | decompile the `Panel*` methods; the names in the panes already make this guessable | 1–2 pw | No, but reduces guesswork |
| U8 | **Mouse capture / focus / modal exact behaviour** | the two UI stacks must not both react to one click | write a contract (this doc + 02) and a test panel; verify against 2026 | 0.5 pw | Yes (design) |
| U9 | **Tooltip timing/placement/sounds** | affects perceived quality, not structure | observe 2026; implement a simple delay + follow cursor | 0.25–0.5 pw | No |
| U10 | **IME composition/candidate placement** | login and chat | reuse `imm32`; compare with 748 `SEditableText` | 0.25–0.5 pw | No |
| U11 | **Per-texture color key / alpha policy** | wrong keying shows black boxes or invisible art | guimat gives a color key; test each of the 49 textures in the previewer | 0.25–0.5 pw | No, but early |
| U12 | **Item icon mapping** (748 `g_itemicon[sIndex]-1` vs 2026 `itemicon.bin`/`itemicon*.wyt`) | item windows look wrong if the atlas differs | choose "keep 748 icons" for S5; remap later | 0.5–1 pw | No (plan for it) |
| U13 | **`wSplitStretch` exact behaviour** | 1 element | infer from draw code or treat as split | 0.25 pw | No |
| U14 | **Animation `Type 3` / `Type 1` exact parameters** | 19 panels fade in/out | inferred from the identical scripts; verify visually | 0.25 pw | No |
| U15 | **2026-only feature/server support** | panels may exist but have no opcode | gate wave 4 on server capability; hide otherwise | product decision | No |
| U16 | **Sound id mapping** (`OpenSound`, `CloseSound`, `ButtonSound`) | part of the panel feel; wrong ids are noticeable | map to the existing sound table; unknown ids play nothing | 0.25 pw | No |
| U17 | **Classic/pane arbitration**: what selects the pane branch in 2026, and which `SControl` controls are hidden when a pane replaces them | the field scene still initializes 1,354 classic controls; drawing both layers is a visible bug | compare the field init with the `Panel*` creation path; instrument 2026 or inspect the selector; implement our own `g_UIVer` + hide list | 0.5–1.5 pw | Yes for S4 |

**Rule:** U1–U2 and U17 must be closed at G0/G1/S4. Everything else can be
deferred with a flag, but U5 and U7 should be closed before S4.

**Note on address bases:** the file's preferred image base is `0x400000`. If
your tool shows `0x676042` / `0xA1BA6C`, it is rebased by `+0x190000`; the file
addresses are `0x4E6042` / `0x88BA6C`. This review used the `C:\Users\Luis\
AppData\Local\Temp\rea-wyd` copy, because the `E:\` copy is rejected by REA.

## 2. Risks

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Pixel parity lost vs 2026 | High | High | Golden captures per panel/resolution; Python previewer; fix 9-slice/scale at G1 |
| Two UI stacks fight for input/focus, or both draw | High | High | Single router in `GuiManager`; explicit focus hand-off; modal stack; **hide the classic controls a pane replaces**; a test panel that logs ownership |
| Item-window regressions (drag/drop, buy/sell, split) | High | High | Do not rewrite `SGridControl` blind; extract behaviour into a `GuiItemGrid` with tests; keep legacy host as fallback; migrate one window at a time |
| Scope creep into an editor | High | Medium | Editor/hot reload is W5, not a prerequisite; ship data + validator first |
| Asset collisions overwrite 748 art | Medium | High | `ui2026/` root + manifest + build check; never copy into `UI/` |
| Text quality/perf regression | Medium | Medium | New `GuiFont` with cached glyphs; keep `TMFont2` untouched; A/B captures |
| Device loss (Alt-Tab) loses pane textures/fonts | Medium | Medium | Implement `Invalidate/RestoreDeviceObjects` in the cache/font; test at G6 |
| No server for field tests | High | High | Stand up a local 748 test server or a packet-replay harness before S4; otherwise S4/S5 estimates are not verifiable |
| `TMFieldScene.cpp` UTF-8 corruption | High | Medium | New code in new files; byte-preserving tools; run `restore_non_utf8.py`; add a CI byte check |
| REA/Ghidra cannot open the binary | Medium | High | Copy `WYD.exe` to a local NTFS path; `rea doctor`; if it still fails, use static disassembly and the existing pseudocode corpus; U1 becomes slower |
| The 2026 client cannot run in this environment (login/region) | Medium | Medium | Use shipped PNG/screenshot references where possible; ask the owner for captures; the format questions can still be answered statically |
| Panels imply features the 748 server does not have | Medium | Medium | Feature flags per panel; never send unknown opcodes; keep the 748 screen behind a flag |
| Maintenance doubles while both stacks exist | High | Medium | Migrate by screen and delete the legacy chrome; time-box the "pane off" switch; do not add new features to the legacy path |
| Deploy size / licensing of 2026 assets | Low | Medium | Ship only referenced assets (~31 MB); confirm redistribution rights with the owner |
| Build breaks from adding 20+ files to the project | Low | Low | Add to `.vcxproj`/`.filters` early; CI build after P1 |

## 3. Decision log

Decisions that should be made explicitly. Defaults are the recommendation; the
"decide by" column is the latest point before the choice becomes expensive.

| ID | Decision | Recommendation | Alternatives | Decide by |
|---|---|---|---|---|
| D1 | Runtime strategy | Native `CGui*` (Option A) | B: translate to `SControl`; C: reskin only | End of P0 |
| D2 | Coexistence | Keep `SControl`/`TMScene`, add pane layer | Rewrite the scene UI | P0 (fixed by evidence) |
| D3 | Item windows | **Keep 748 grids for the first release**; native `GuiItemGrid` for inventory/cargo afterwards; legacy host for trade/shop/mix if needed | All native now; all legacy forever | End of S4 |
| D4 | Text | New `GuiFont` over D3DXFont; keep `TMFont2` | Extend `TMFont2`; FreeType atlas | P2 |
| D5 | Scale policy | 800×600 design space, anchors + uniform scale; verify at 1024 | 800×600 only; per-panel scale | G0/G1 |
| D6 | Asset layout | `client_748/ui2026/` with a resolver root | Replace `UI/`; separate client install | P1 |
| D7 | Localization | Separate 2026 EN table beside 748 `UIString.txt` | Merge/overwrite 748 table | P1 |
| D8 | Scope | **S4 first release** (pane login/chrome/HUD/chat + 748 item grids); S5 optional; S6 later | S2 only; S5 now; full S6 | After S2 |
| D9 | Hot reload | W5, unless iteration pain justifies earlier | Never; editor | After S3 |
| D10 | Legacy fallback | Keep a "pane off" switch for one release | Remove legacy immediately | S3 |

## 4. Go / no-go

| Point | Question | If no |
|---|---|---|
| Before P0 | Is the team willing to spend 24–38 pw on a runtime, not just a reskin? Is 2026 art acceptable with 748 gameplay? | Choose Option B (10–14 pw) or C (3–5 pw); do not start P1 |
| After S1 (G1) | Do `notice`/`msgbox` render within ±2 px with correct text/textures? | Stop screen work; fix 9-slice, scale, fonts, color key |
| After S2 (G2) | Are hover/press/focus/modal and legacy arbitration clean? | Stop; redesign the input router; do not migrate screens |
| After S3 (G3) | Is login → char select stable and visually acceptable? | Decide whether to keep the login legacy and continue only with the field |
| After S4 (G4) | Does the field run with the pane HUD/chat, with the replaced classic HUD controls hidden, and remain playable? | Fix suppression/input before touching item windows |
| After S5 (G5) | Do item flows pass the regression checklist? | Keep the legacy host for the failing window; do not ship a half-migrated inventory |

## 5. Open questions for the owner

1. **Look vs behaviour:** is the goal the 2026 look with 748 behaviour, or should
   new 2026 features (collection, arena, refine, config) be implemented too?
2. **Primary resolution:** 800×600 only, or must it work at 1024×768 and above?
3. **Server:** is there a 748 test server (or packet replay) to verify the field
   and item flows? Without it, S4–S5 cannot be gated.
4. **Art approval:** is mixing 2026 UI art with 748 world/character art
   acceptable? Screens without a pane (firework/toto/gamble/quiz/mission) will
   remain 748-styled.
5. **Text policy:** is the modern TTF look mandatory, or is the 748 bitmap font
   acceptable for some screens?
6. **Delivery shape:** one big UI release, or incremental screens behind flags?
7. **Legal:** may the 2026 UI assets and fonts be redistributed with the client?

## 6. What not to do

- Do not attempt a big-bang replacement of `TMScene`/`SControl`. The 2026 client
  kept both: its scene factory still instantiates `TMFieldScene` and loads
  `FieldScene2.bin` (1,354 controls).
- Do not treat the `.bin` scene as dead data. It is live, initialized, and
  larger than the 748 scene.
- Do not draw or hit-test a pane and its classic equivalent at the same time;
  hide one explicitly.
- Do not put panes into the 30-layer `m_pDrawControl` list; use the pane
  compositor.
- Do not reuse the 512 `TextureManager` UI slots for guimat textures.
- Do not overwrite 748 textures/strings with 2026 files of the same name.
- Do not rewrite `SGridControl` before the panel layer exists and the first
  windows are proven.
- Do not build unused widgets (Slider, ComboBox, TreeView, ImageFont, MoMesh)
  speculatively.
- Do not make the editor/hot reload a prerequisite; data + validator first.
- Do not touch `TMFieldScene.cpp` with a UTF-8-only editor. It contains 13 raw
  non-ASCII bytes on 9 lines.
- Do not call the work done when the panels render; the gate is that the **game
  flows** still work.

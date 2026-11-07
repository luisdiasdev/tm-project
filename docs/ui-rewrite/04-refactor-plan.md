# 04 — Refactor Plan and Effort

This is a **strangler refactor**: build the new data/binding/layout layers
around the existing widgets and renderer, migrate one screen at a time, and
delete the old path when a screen is done. No big-bang cutover.

## Guiding principles

1. **Keep the renderer.** D3D9, atlas blitting, color modulation, and the bitmap
   font stay. Replacing them is a different project with a different risk
   profile.
2. **Keep the widgets.** `SPanel`, `SButton`, `SListBox`, `SGridControl`,
   `SEditableText`, … already model the game. Reuse them behind a new interface.
3. **The `.bin` stays as the oracle.** JSON is generated from it, and a
   round-trip test must reproduce it byte-for-byte. This keeps the original
   client as the ground truth.
4. **Names are the API.** Code binds by stable name (`"inventory.grid"`), never
   by number.
5. **Fail loudly.** A missing control, duplicate ID, unknown type, missing
   texture set, or bad action is an error at load/validate time, not a silent
   no-op.
6. **Parity is a gate, not a hope.** Every phase ends with a screenshot
   comparison against `client_748/WYD.exe`.

## Target architecture

```
  scenes/field.ui.json          themes/wyd748.json         strings/en.json
        │                             │                          │
        └────────────┬────────────────┴──────────────┬───────────┘
                     ▼                               ▼
              schema validator                 string table
                     │
                     ▼
        UiDocument  (immutable, loaded once)
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
  LayoutEngine              BindingRegistry
  (anchors, z, 9-slice)     (name → SControl*, typed)
        │                         │
        ▼                         ▼
  SControl tree  ◀──────  UiScene (code)
        │                         │
        ▼                         ▼
  FrameMove2 → draw list    ActionRegistry (named handlers)
        │
        ▼
  D3D9 renderer (unchanged)
```

Three new subsystems, all additive:

- **`UiDocument`** — parsed JSON, validated, resolved (text keys → strings,
  style refs → values, anchors → rectangles at the current resolution).
- **`BindingRegistry`** — typed, name-based lookup with a one-time validation
  pass that reports every missing/wrong-type binding.
- **`ActionRegistry`** — maps action names to small C++ functions; the JSON can
  reference them, but cannot execute arbitrary logic.

## Effort model

Units: **developer-weeks** for one engineer who knows the codebase. Low = no
surprises, high = includes the usual decompiled-code archaeology. The estimate
includes tests and parity checks, not product decisions.

### Quick wins (do these first, independently)

| # | Item | Effort | Delivers |
|---|---|---:|---|
| Q1 | Run the existing audit as a build gate (dead refs, duplicate IDs, unnamed IDs, missing texture sets) | 0.5–1 wk | Every misbind becomes a build error |
| Q2 | Dear ImGui overlay: hover inspector (ID/name/type/rect/set/parent/layer), reload button | 1–1.5 wk | You can see what you are clicking; troubleshooting drops sharply |
| Q3 | Typed `Bind<T>(name)` + one-time missing-control report | 0.5–1 wk | Removes casts and 104 hand-written null guards over time |
| Q4 | Name the 16 unnamed field controls; resolve the 15 duplicate constant values | 0.5 wk | Removes a class of ambiguity |
| Q5 | Prototype hot reload of the current `.bin` (re-parse + rebuild tree, keep state) | 1–2 wk | Cuts iteration from minutes to seconds before any format change |
| | **Total** | **3.5–6 wk** | |

Q1+Q2+Q3 are independent and can be done in any order. They do not require the
JSON work and they pay for themselves during it.

**If you do only one thing this week:** build the ImGui hover inspector (Q2).
It converts "why is this wrong" from a guessing game into a lookup, and it is
the cheapest possible attack on the "hard to troubleshoot" complaint.

**If you do only one thing this month:** Q1 + Q2. Together they make every
existing misbind visible and every click identifiable, without touching the
format or the runtime.

### MVP — JSON scenes + typed bindings + hot reload

| Phase | Work | Effort | Exit criteria |
|---|---|---|---:|
| 0 | Freeze baseline: screenshot set at 800×600 for login/server/char/field, plus a golden `WYD.log` | 1–2 wk | Repeatable capture script; parity harness exists |
| 1 | `bin ↔ json` converter, JSON Schema, validator, byte-identical round-trip test for all three scenes | 2–3 wk | `convert --check` passes in CI |
| 2 | Runtime `UiDocument` loader, name registry, `Bind<T>`, explicit `z`, basic anchors, modal stack | 4–6 wk | Field scene loads from JSON and renders identically at 800×600 |
| 3 | Codegen: typed accessors + init skeleton from JSON; migrate `InitializeScene` for all three scenes | 4–6 wk | Zero numeric `FindControl`; every binding validated at startup |
| 4 | Hot reload (watch + rebuild subtree + preserve state) and ImGui inspector integration | 2–3 wk | Edit JSON → visible change in < 2 s without restart |
| | **MVP total** | **13–20 wk** | |

Phase 3 is the riskiest estimate. Codegen removes the 375 mechanical
`FindControl` calls, but the ~1,200 remaining lines of per-control fixes and
runtime-built panels must be ported by hand. If that turns out to be more
entangled than it looks, add **2–3 weeks**.

These estimates exclude art/design work (choosing frames, re-skinning panels)
and product decisions about which 755-only features to drop.

After the MVP, the original complaints are largely addressed:

- layout/art/text/visibility changes are data edits;
- misbinds are build/load errors;
- troubleshooting has an inspector;
- iteration is seconds.

### Core — presentation quality

| Phase | Work | Effort | Exit criteria |
|---|---|---|---:|
| 5 | Layout engine completion: anchors, percentage, margins, 9-slice, DPI-independent units, resolution matrix | 2–3 wk | Same JSON renders correctly at 800×600, 1024×768, 1280×960, 1600×1200 |
| 6 | Text pipeline: glyph atlas cache, batched draw, real wrapping, color markup, CJK/IME, keep the 748 glyph look | 3–5 wk | No GDI call on the render path; long text wraps; visual match at native size |
| | **Core total** | **5–8 wk** | |

### Full — authoring workflow

| Phase | Work | Effort | Exit criteria |
|---|---|---|---:|
| 7 | Declarative actions: schema + runtime (`show`, `hide`, `toggle`, `setText`, `selectTab`, `playSound`, `sendPacket`, `runHandler`); migrate the ~91 simple field branches | 4–6 wk | Adding a panel open/close is a JSON edit |
| 8 | Editor MVP (web): load JSON + atlas preview, drag/resize, tree/z editing, text editing, validation panel, save | 5–8 wk | A non-programmer can move a panel and commit the diff |
| 9 | Parity hardening: every screen, every resolution, regression suite, docs | 4–6 wk | Full checklist green; no known regressions |
| | **Full total** | **13–20 wk** | |

### Roll-up

| Scope | Effort | Cumulative |
|---|---:|---:|
| Quick wins | 3.5–6 wk | 3.5–6 wk |
| MVP | 13–20 wk | 16.5–26 wk |
| Core | 5–8 wk | 21.5–34 wk |
| Full | 13–20 wk | **34.5–54 wk** |

That is a large range. It is honest: the difference between "a data-driven
runtime that removes the pain" (MVP, ~3–5 months) and "a complete authoring
pipeline with an editor" (full, ~8–12 months) is mostly editor and parity work,
not core engineering.

**Recommended stopping point: Core (MVP + layout + text), ~5–8 months.** The
editor can be built later against a stable schema, and only if non-programmers
will actually use it.

## Migration order

1. **Server select** (22 controls) — smallest, already code-built, good pilot.
2. **Character select** (85 controls) — medium, exercises lists and edits.
3. **Field** (709 controls) — the bulk; do it in slices:
   - HUD/chrome (chat, main info, minimap, skill belt)
   - inventory/cargo/equipment grids
   - trade/shop/mix/mission panels
   - event/gamble/toto/firework popups

Do not migrate all 709 at once. Each slice must pass the parity gate before the
next starts.

## Testing and parity gates

| Gate | After | Pass condition |
|---|---|---|
| G0 | Phase 0 | Baseline captures reproducible; diff tooling works |
| G1 | Phase 1 | Round-trip is byte-identical for all three `.bin` files |
| G2 | Phase 2 | Field scene from JSON is pixel-identical at 800×600 |
| G3 | Phase 3 | All three scenes migrated; zero numeric `FindControl`; startup binding report is clean |
| G4 | Phase 4 | Hot reload < 2 s; inspector shows correct metadata |
| G5 | Phase 5 | Resolution matrix matches; no per-control resolution branches |
| G6 | Phase 6 | Text visual match; no GDI on render path; wrapping verified |
| G7 | Phase 7 | Simple branches migrated; handler registry covers the rest |
| G8 | Phase 9 | Full regression checklist green |

A gate is a hard stop. If G1 cannot be met byte-for-byte, the schema is wrong and
must be fixed before proceeding — that is the cheapest possible place to learn
it.

## Risks and mitigations

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Pixel parity lost | High | High | Golden captures per screen/resolution; overlay diff tool; migrate in slices |
| Scope creep into an editor | High | Medium | Editor is explicitly Phase 8; ship MVP without it |
| Text pipeline changes the look | Medium | High | Keep the bitmap glyph atlas as the default; add batching underneath; A/B captures |
| Grid drag/drop regressions | Medium | High | Do not rewrite `SGridControl`; wrap it and migrate its call sites only |
| Hidden coupling: gameplay reads UI pointers (`m_pGridInv`, `m_pChatList`, …) | High | Medium | Introduce accessor facade (`Ui.field.inventoryGrid`) and migrate call sites gradually |
| JSON parse cost | Low | Low | Parse once at scene load; cache; 165 KB is negligible |
| Non-UTF-8 `TMFieldScene.cpp` corrupts during edits | High | Medium | Use the byte-preserving tools in `docs/tools/`; check the 13-byte/9-line invariant; move UI code out of that file early |
| New dependency (JSON parser) | Low | Low | Vendor a single header (`nlohmann/json`) or a small parser; no package manager needed |
| Team unfamiliar with the new schema | Medium | Medium | JSON Schema + generated docs + validator messages |
| Divergence from original `.bin` | Medium | High | Keep `.bin` as oracle; round-trip CI; never edit `.bin` by hand after migration |

## Go / no-go

- **Before starting:** confirm the MVP scope (13–20 wk) is acceptable and that
  parity at 800×600 is the primary acceptance criterion.
- **After Phase 1:** if round-trip cannot be byte-identical, stop and fix the
  schema. Do not continue on an approximate converter.
- **After Phase 2:** if the JSON-loaded field scene is not pixel-identical,
  stop and fix the loader before migrating more screens.
- **After Phase 4:** decide whether layout/text (Core) is worth another 5–8 wk,
  or whether the MVP already solves the day-to-day pain.
- **After Phase 6:** decide whether an editor is justified by actual users.

## What not to do

- **Do not** replace the widgets or the renderer in the first pass.
- **Do not** do a big-bang cutover; the field scene alone has 709 controls and
  125 grids.
- **Do not** use YAML for scene data (indentation-sensitive, heavier parser,
  weaker tooling).
- **Do not** add Qt or a browser runtime.
- **Do not** try to make gameplay logic declarative; it will become a worse
  scripting language than C++.
- **Do not** edit `TMFieldScene.cpp` with a UTF-8-only editor; it is not valid
  UTF-8 (13 raw bytes on 9 lines).

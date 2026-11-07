# 03 — Evaluating the Proposals

Three ideas were proposed:

- **A. Modern / consolidated framework** instead of the hand-rolled system.
- **B. More dynamic**, so interface changes do not require code changes.
- **C. A new format, possibly JSON/YAML directly.**

They are not alternatives. B is the goal, C is the main enabler, A is a
different (and mostly worse) way to pursue B. This document scores each.

## Scoring rubric

Each proposal is scored 1–10 on four axes:

| Axis | Meaning |
|---|---|
| **Impact** | how much of the four complaints (changeability, error-proneness, troubleshooting, time) it removes |
| **Cost** | engineering effort (10 = most expensive) |
| **Risk** | chance of regression, lost parity, dead end (10 = worst) |
| **Score** | overall recommendation for *this* project, not in general |

---

## Proposal A — Use a modern, consolidated UI framework

### A1. Dear ImGui

Immediate-mode, MIT, official Win32 + **D3D9** backends
(`imgui_impl_win32.cpp` + `imgui_impl_dx9.cpp`), integration is a few dozen
lines.

| | |
|---|---|
| Impact | 3 as the game UI, 8 as a tool |
| Cost | 2 (overlay), 9 (full game UI) |
| Risk | 2 (overlay), 8 (full game UI) |

**Why it fails as the game UI.** ImGui is immediate-mode: the layout *is* the
C++ code. That is the exact problem you want to escape. You would trade a
1,632-line `InitializeScene` for an equally large `RenderHUD()` with no data
format, no designer workflow, and no parity with the 748 art. Drag/drop item
grids, IME text entry, and the bitmap font would all be re-implemented.

**Why it wins as a tool.** A debug overlay that shows the control under the
cursor (ID, name, type, rect, texture set, parent, draw layer/order) would
remove most of the troubleshooting pain for **2–3 days** of work. It is the
highest ROI item in this entire document.

### A2. RmlUi (HTML/CSS retained mode)

MIT, C++17, data-driven RML + CSS, data binding, hot reload, animations. It is
the closest thing to "a modern consolidated framework" that fits the goal.

| | |
|---|---|
| Impact | 8 |
| Cost | 9 |
| Risk | 8 |

**Blockers specific to this project:**

- **No first-party D3D9 renderer.** Official backends are GL2/GL3, Vulkan,
  DX11, DX12, SDL GPU. You must write and maintain a D3D9 `RenderInterface`
  (roughly 500–900 lines) including clip/scissor, blend state, and texture
  management.
- **Texture pipeline mismatch.** RmlUi wants images and `@font-face`; this game
  has `.wyt` atlases addressed by *set index + frame index*, and a bitmap font
  built from GDI. You would write a custom image/font provider and re-encode
  the atlas.
- **IME.** The client has a hand-written IME path
  (`EventTranslator`, `SEditableText`). RmlUi has IME hooks, but wiring them to
  the existing Win32/DirectInput loop is real work.
- **Grid drag/drop.** Inventory/trade/cargo grids are 125 of the 709 field
  controls and carry the most complex interaction in the game. RmlUi has no
  equivalent; you would embed a custom widget and lose most of the benefit.
- **Parity.** Every screen would be re-authored in HTML/CSS and would have to be
  pixel-matched to the 748 client. That is a design project, not a port.
- **Modding/asset compatibility.** `.wyt` atlases and scene bins are the
  community's data. RmlUi moves the UI out of that ecosystem.

**Verdict.** Technically viable, but you would spend most of the effort
re-creating what you already have, and the result would look different unless
you constrain CSS to exactly the old layout. Not recommended for the game
screens. It could be reconsidered for a *new* launcher/patcher/settings tool.

### A3. Qt / QML

| | |
|---|---|
| Impact | 7 |
| Cost | 10 |
| Risk | 9 |

Wrong tool. Qt would replace the window, input, renderer, and event loop; LGPL
or commercial licensing applies; the binary grows by tens of MB; and the D3D9
game view would become an embedded surface. It solves none of the
binding/behavior problems and adds a large dependency to a 2007-era client.

### A4. Web UI (CEF / WebView2 / Ultralight)

| | |
|---|---|
| Impact | 7 |
| Cost | 10 |
| Risk | 9 |

Same parity and input problems as RmlUi, plus a browser runtime and IPC. A
non-starter for an in-game HUD.

### A summary

| Option | As game UI | As dev tool |
|---|---|---|
| Dear ImGui | ✗ (immediate mode = code layout) | ✓✓ recommended |
| RmlUi | △ (parity/atlas/IME/grid risk) | — |
| Qt/QML | ✗ | △ (separate tools) |
| Web UI | ✗ | △ (separate tools) |

**Score for A as the primary UI: 3/10.**
**Score for A as a developer overlay (ImGui): 9/10.**

---

## Proposal B — Make the UI more dynamic

Goal: "we don't need to change code to handle interface changes."

This is the right goal. The question is scope. Split "interface change" into
three tiers:

| Tier | Examples | Can it be data? |
|---|---|---|
| **Presentation** | position, size, anchor, texture set/frame, color, font, text, visibility default, draw order, grid shape, tab group | **Yes, 100%** |
| **Simple interaction** | show/hide/toggle panel, select tab, set text, enable/disable, play sound, send a fixed packet, close modal | **Yes, ~65% of field handlers** |
| **Game behavior** | inventory page math, item validation, drag/drop, chat commands, combat, server flows | **No — stays code, but becomes small named handlers** |

Evidence for the 65% figure: `TMFieldScene::OnControlEvent` has 139 top-level
branches; 91 contain no packet send, no `g_pObjectManager` mutation, no sound, no
grid/reel/skill logic. Their median length is 12 lines and most are
open/close/select/set-text.

**What this buys.** A designer can add a panel, move it, skin it, reorder it,
change its text, and wire its open/close behavior without a rebuild. When a new
*interaction* is needed, you write one small handler and register it by name —
you do not edit a 3,521-line function.

**What it does not buy.** You cannot make "sell an item from the cargo grid"
data-driven without building a scripting layer, and that layer would be larger
and riskier than the code it replaces.

**Score: 7/10** as stated (too broad), **9/10** if scoped to the three tiers
above.

---

## Proposal C — New format, JSON/YAML directly

### C1. JSON

| | |
|---|---|
| Impact | 8 |
| Cost | 4 |
| Risk | 3 |

**Pros.**

- Human-readable and diffable; Git review becomes meaningful.
- Every language has a parser; a web editor is trivial.
- Schema validation is mature (JSON Schema).
- C++ side: `nlohmann/json` is a single MIT header, or `rapidjson` if compile
  time matters. The project is already **C++17** (`stdcpp17` in
  `TMProject.vcxproj`), so either works.
- Comments are the only real loss. Solve with a `"_comment"` key or JSON5/JSONC
  if the parser supports it.
- Size is a non-issue: the field scene converts to **~165 KB** pretty-printed
  (~90 KB minified) from 35 KB binary.

**Cons.**

- Hand-editing 709 nested controls is still unpleasant without an editor.
- JSON has no notion of units, anchors, or z-order — those are *your* schema
  decisions, not the format's.

### C2. YAML

| | |
|---|---|
| Impact | 7 |
| Cost | 4 |
| Risk | 5 |

**Pros.** Comments, less punctuation, nicer for hand edits.

**Cons.**

- A C++ YAML parser (`yaml-cpp`) is a real library with a build step, unlike a
  header-only JSON parser. The project currently has no package manager and only
  a vendored DirectX SDK.
- Indentation-sensitive syntax is a poor fit for 709 machine-generated records;
  a stray space changes the tree.
- Anchors/aliases are powerful but become an implicit inheritance system that is
  hard to validate.
- Tooling (schema validation, web editors) is weaker than JSON.

**Verdict.** JSON. Use YAML only if a human-authored config layer (themes,
keybinds) is added later, and keep it separate from scene data.

**Score: JSON 8/10, YAML 6/10.**

### C3. What the format must contain (beyond a 1:1 dump)

A naive `.bin → JSON` conversion is not enough. To deliver B, the schema needs:

- **names**, not just numeric IDs;
- **explicit `z`** (or an ordered children array) so ordering is visible;
- **anchors / percentage / margins** so layout survives resolution changes;
- **text keys or inline text**, not `stringIndex`;
- **style tokens** (a `style` reference) instead of repeating colors;
- **event bindings** (`onClick: { action: ... }`);
- **visibility groups / screens** so a panel and its children can be toggled as
  a unit.

[`05-target-format.md`](05-target-format.md) specifies this concretely.

---

## Combined recommendation

Do **B + C**, with **A only as a developer overlay**.

```
                 ┌──────────────────────────────┐
                 │  JSON scene + theme + strings │  authoring format
                 └──────────────┬───────────────┘
                                │ validator + codegen + hot reload
                                ▼
        ┌───────────────────────────────────────────────┐
        │  typed binding registry (name → widget)        │
        │  layout engine (anchors, z, 9-slice)           │
        │  action registry (named handlers)              │
        └──────────────┬────────────────────────────────┘
                       │
                       ▼
        ┌───────────────────────────────────────────────┐
        │  existing SControl widgets + D3D9 renderer     │
        └───────────────────────────────────────────────┘
```

Keep the widgets and the renderer; replace the data, binding, layout, and
behavior layers around them. That is a **strangler refactor**: the new system
grows around the old one, and screens migrate one at a time.

### Final scores

| Item | Score |
|---|---:|
| A. Framework as primary UI | 3/10 |
| A. ImGui as developer overlay | 9/10 |
| B. Dynamic UI (scoped) | 9/10 |
| C. JSON format | 8/10 |
| C. YAML format | 6/10 |
| **B + C + ImGui overlay (recommended)** | **9/10** |
| Big-bang rewrite on a new framework | 2/10 |

The next document turns the recommendation into phases with effort and gates.

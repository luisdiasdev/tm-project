# 02 — Pain Points, With Evidence

The brief names four complaints: **not friendly to changes, error-prone, hard to
troubleshoot, time-consuming**. This document turns each into specific,
measured failure modes.

Legend for "Fixed by": **F** = new file format, **B** = binding registry,
**L** = layout engine, **T** = text pipeline, **H** = hot reload/tooling,
**D** = declarative actions. See [`03-proposals.md`](03-proposals.md).

---

## P1. The format is write-only

**Evidence.** `UIBinary.h` defines nine fixed-size structs. A scene record is
`int32 type` + a body whose layout depends on the type. There is no header,
version, name, comment, or checksum (`Projects/TMProject/UIBinary.h`).

**Consequence.** You cannot look at a `.bin` and know what a control is. You
must cross-reference `ResourceControl.h`, and even that is ambiguous (P3). There
is no writer in the repo — `docs/tools/ui_inventory.py` only reads. Changing a
pixel means either a hex editor or a rebuild of the whole UI in code.

**Cost.** Every layout tweak is a code change. That is the root of "not friendly
to changes".

**Fixed by:** F.

---

## P2. Ordering is invisible and inverted

**Evidence.** `TreeNode::AddChild` prepends (`TreeNode.cpp:120`). The loader adds
children as it reads records. `FrameMove2` traverses `m_pDown` first. Net effect:
**the last record for a parent renders behind its earlier siblings**.

This is not a theory. `docs/port-748/message-box.md` explicitly says:

> The background must be added **last** so it becomes the first child and
> renders behind texts and buttons.

**Consequence.** Z-order is encoded in file order, with an inversion. A designer
cannot say "this is on top"; they have to count records. Reordering a `.bin` to
fix one overlap can break several others. Runtime-created controls get the
opposite convention from file controls, so mixed panels are especially fragile.

**Cost.** Overlap bugs are diagnosed by trial and error.

**Fixed by:** F (explicit `z`), L.

---

## P3. IDs are magic numbers, and they collide

**Evidence.** `Projects/TMProject/ResourceControl.h` has **2,197** `constexpr`
constants. **1,177** are `TM*` (748) names; the rest are 755/760 names. The
header contains **15 duplicated values** (two or more names with the same
number), for example:

| Value | Names |
|---:|---|
| 306 | `TMB_PKBTN`, `TMP_TITLE_PANEL2` |
| 307 | `TMB_NAMEBTN`, `TMP_TITLE_PANEL3` |
| 312 | `TMB_AUTOTARGETBTN`, `TMP_LOGO_PANEL2` |

Scene IDs are not unique either: `FieldScene2.bin` has two records with ID
`6192`, `SelCharScene2.bin` has two with `631`. `FindControl` returns the first
match, so the second is unreachable by ID.

The same numeric ID means different things in different scenes. `626/627` are
`TMP_INPUT_GOLD`/`TME_INPUT_GOLD` in the field scene but "Enter Password" in
`SelCharScene2` (`docs/port-748/control-ids.md`).

**Consequence.** `FindControl(627)` can succeed and return the wrong widget. The
audit reports **0 type mismatches** today only because the 229 remapped bindings
were audited by hand.

**Cost.** The 748 port spent significant effort on exactly this: 264 of 453
numeric references in `TMFieldScene.cpp` pointed at IDs absent from
`FieldScene2.bin`, and 229 had to be remapped by script.

**Fixed by:** B.

---

## P4. Binding is ~330 raw pointers and 1,632 lines of init

**Evidence.**

| Metric | `TMFieldScene.cpp` | `TMSelectCharScene.cpp` | `TMSelectServerScene.cpp` |
|---|---:|---:|---:|
| Lines | 24,638 | 2,237 | 1,554 |
| `FindControl` calls | 511 | 99 | 17 |
| …with numeric literal | 273 | — | — |
| `SetVisible` calls | 621 | 84 | 44 |
| Pointer members in header | ~330 | ~90 | ~40 |
| `InitializeScene` length | 1,632 | 338 | 263 |
| `if (m_pX)` null guards in init | 104 | — | — |

**Consequence.** Every control you touch needs a member, a lookup, a cast, and
usually a null guard. A typo in the ID compiles fine and fails at runtime. A
control that is removed from the scene turns into a null dereference unless
someone remembered a guard.

**Cost.** This is the bulk of the "time-consuming" complaint. Adding one button
to a panel touches a header and at least one 1,600-line function.

**Fixed by:** B, F (names in data), H (generated bindings).

---

## P5. Missing controls fail at runtime, not at build time

**Evidence.** The audit of `FieldScene2.bin` against `TMFieldScene.cpp`
(`tools/fieldscene/out/fieldscene-audit.json`):

| Metric | Count |
|---|---:|
| Source references | 699 |
| Resolved to a scene control | 465 |
| Dead values (IDs not in scene) | 127 |
| Dead references (occurrences) | 164 |
| …755-only | 99 |
| …unresolved | 26 |
| …dynamic (created at runtime) | 2 |
| Controls never referenced by source | 308 |
| Scene IDs with no header name | 16 |
| Duplicate constant values (in used references) | 10 |
| Type mismatches | 0 |

**Consequence.** 164 lookups return `nullptr`. They are "handled" by 104 null
guards spread through init and by ad-hoc early returns elsewhere. The 0x114 crash
chain during the 748 port (`docs/port-748/remaining-work.md`) was a sequence of
exactly these: `SetText` on a missing control, `Empty` on a missing grid, a
missing page grid dereferenced in a carry loop.

**Cost.** Crashes are found by playing, not by building. Each one costs a
rebuild + relogin + navigate cycle.

**Fixed by:** B (typed lookup logs/throws), H (validator in CI).

---

## P6. Layout and resolution handling are ad hoc

**Evidence.**

- `SControl::SControl` scales position and size by `screen/800` and
  `screen/600` once, at construction (`SControl.cpp:31-36`).
- `BASE_ScreenResize(size)` scales by **width only**: `screenW * size / 800`
  (`Basedef.cpp:33`).
- Three scaling conventions coexist: the constructor scales by both ratios;
  `BASE_ScreenResize` scales by width only; `SetPos`/`SetSize` do not scale at
  all. Some code uses `490.0f * RenderDevice::m_fWidthRatio` directly instead
  of either helper. Which convention applies depends on which path a control
  took, so the effective scale is not uniform.
- `SetCenterPos` centers only X, and only for five hard-coded IDs
  (`SControl.cpp:228`).
- `TMSelectServerScene::InitializeScene` has a per-resolution branch with a
  typo: `else if (g_pDevice->m_dwScreenWidth == 12180)` (`TMSelectServerScene.cpp:174`)
  — should almost certainly be `1280`.
- `RenderGeomRectImage` special-cases `nTextureSetIndex == 526` for button text
  placement (`RenderDevice.cpp:3299`).

**Consequence.** The UI is correct at 800×600 and drifts at other resolutions.
Fixing it requires more per-control code, not less.

**Cost.** Multi-resolution support is effectively a separate project.

**Fixed by:** L (anchors, percentage, 9-slice, DPI-independent units).

---

## P7. Text is rasterized with GDI on every change

**Evidence.** `TMFont2::SetText` (`TMFont2.cpp:28`) caps at 120 chars, splits
into three ~42-char chunks, calls `TextOut` into a shared DIB, then locks and
uploads a D3D9 texture. Each `SText` owns four `TMFont2` objects.

**Consequence.**

- Blurry text at non-native scale.
- Hard 120-character limit (the message panel was previously truncated at 82;
  see `docs/port-748/message-box.md`).
- Chunk seams at ~42 characters.
- No real word wrapping; `SMessageBox::SetMessage` splits at 34 characters.
- No Unicode beyond the active code page; IME is a separate hand-written path.
- CPU cost per changed label per frame.

**Cost.** Any text-heavy screen (chat, quest log, tooltips) is a performance and
polish risk.

**Fixed by:** T.

---

## P8. Style and behavior are hard-coded in generic classes

**Evidence.**

- `SPanel::OnMouseEvent` returns 0 for IDs `65943`/`65947` (`SControl.cpp:265`).
- `SPanel::SetVisible` has a latent bug: when a *picked* panel is hidden it sets
  `g_pCurrentScene->m_pControlContainer = nullptr` (`SControl.cpp:361`) instead
  of clearing `m_pPickedControl`. If reached, the whole UI container is lost.
- `RenderDevice::RenderGeomRectImage` hard-codes set `526` for text placement
  and a list of mesh-index ranges for `RENDER_3DOBJ`
  (`RenderDevice.cpp:3317-3336`).
- `SGrid.cpp:274` derives an inventory page number from a control ID:
  `int page = pFScene->m_pGridInv->m_dwControlID - 67072;` — a 755-only layout
  encoded as arithmetic on IDs.

**Consequence.** Widgets cannot be reused without inheriting the quirks of a
specific screen. Fixing a generic class risks breaking unrelated screens.

**Cost.** Every new screen re-implements workarounds.

**Fixed by:** B, D, L.

---

## P9. Modality is eight hand-assigned slots

**Evidence.** `SControl* m_pModalControl[8]` (`SControlContainer.h:39`). Scenes
assign specific indices: message box `0`, password dialog `1`, system panel `1`,
PGT panel `2`, input-gold `3`, second message box `4`, firework `5`, toto `6`
(`TMScene.cpp:200`, `TMFieldScene.cpp:1198-1216`, `TMSelectCharScene.cpp:216`).
`OnMouseEvent` scans the eight slots in order and takes the first visible modal.

**Consequence.** Two modals in the same slot silently shadow each other. Slot
numbers are a hidden dependency between scenes. There is no modal stack.

**Cost.** Modal bugs are hard to reproduce and depend on which panel was opened
first.

**Fixed by:** L (modal stack as part of the framework).

---

## P10. Iteration is rebuild + relaunch + relogin

**Evidence.** `TMScene::ReloadScene()` exists but is never called
(`TMScene.cpp:1474`; no other references). There is no file watcher, no
in-game UI inspector, and `SControlContainer::GenerateText` is commented out
(`SControlContainer.cpp:350`).

**Consequence.** To see one pixel move you must:

1. edit C++,
2. rebuild (and the PCH / `LNK1168` rules make this fragile),
3. relaunch the client,
4. log in (`admin`/`admin`), pick the 4th server, pick a channel, connect,
5. select a character, enter the field,
6. open the panel you are working on.

**Cost.** This is the dominant cost of every UI task, and it is pure overhead.
It also makes "try three positions and pick one" impractical.

**Fixed by:** H.

---

## P11. No validation, no invariants, no tests

**Evidence.** The loader:

- aborts the whole scene on an unknown control type (`TMScene.cpp:807`),
- returns 0 on a short read and leaves a partially built tree,
- never checks for duplicate IDs,
- never checks that a parent exists before a child (it silently drops the child
  into the void if the parent lookup fails — the control is created but never
  added to the tree),
- never checks that a texture set exists (missing set renders nothing).

The only automated check is `docs/tools/ui_inventory.py` and the untracked
`tools/fieldscene` audit; neither runs in the build.

**Consequence.** The three shipped scenes happen to parse cleanly, but a
hand-edited file can fail in five different silent ways.

**Cost.** Debugging starts from "nothing renders" with no diagnostic.

**Fixed by:** F (schema), H (validator in CI).

---

## P12. Text/localization is split three ways

**Evidence.** See §2.3 of [`01-current-architecture.md`](01-current-architecture.md).
`g_UIString` is `char[500][64]` and parsed with `%d %s`
(`TMGlobal.cpp:30`, `Basedef.cpp:1191`), so `C.C Mode(MC)` becomes `C.C`.
Messages live in an XOR-encrypted `strdef.bin` referenced by index from code.
Some labels are string literals.

**Consequence.** Translating or rewording a label can require editing a `.txt`,
a binary blob, or C++ — sometimes all three.

**Cost.** Localization is a code project, not a content task.

**Fixed by:** F (inline text keys), T (rich text).

---

## P13. Fixed capacities and silent truncation

**Evidence.**

| Limit | Value | Location |
|---|---:|---|
| Draw layers | 30 | `SControlContainer.h:7` |
| Modal slots | 8 | `SControlContainer.h:39` |
| Listbox items | 1,000 | `SControl.h:560` |
| UI strings | 500 × 64 B | `TMGlobal.cpp:30` |
| Message table | 2,000 × 128 B | `Basedef.h:6` |
| Text length | 120 | `TMFont2.h:3` |
| Grid items | 128 | `SGrid.h:100` |

**Consequence.** Exceeding any of these is silent. A deeply nested panel simply
does not draw; a long chat line is cut.

**Cost.** Rare, but extremely hard to diagnose when it happens.

**Fixed by:** F/L/T (dynamic containers).

---

## Aggregate: what a UI change costs today

A typical "move this panel 10 px and add a button" task:

| Step | Time | Why |
|---|---|---|
| Find the control | 10–30 min | no names in data; search the 2,197-constant header and the scene |
| Add/adjust the `.bin` | 15–60 min | no writer; hex edit or rebuild in code |
| Add the ID constant | 2 min | but risk a duplicate value |
| Add the member + `FindControl` | 5–15 min | plus null guard |
| Add the click handler | 5–60 min | 3,521-line function |
| Rebuild + relaunch + relogin + navigate | 3–10 min | P10 |
| Verify and repeat | ×N | no hot reload |

So a trivial change is **30–90 minutes**, most of it mechanical. A JSON format
with hot reload and typed bindings reduces the mechanical part to seconds; the
design decision remains.

## What is actually good and should be kept

It is important not to throw away the parts that work:

- **Texture sets** (`UITextureSetList.txt`) are already a clean, declarative
  atlas description.
- **The `SControl` widget set** is complete for this game (panel, button, check,
  radio, list, edit, progress, scroll, grid, 3D object, reel).
- **The grid/item model** (`SGridControl` + `SGridControlItem` + cursor
  attachment) encodes real drag/drop semantics that would be expensive to
  re-create.
- **The bitmap font** is part of the 748 look; replacing it changes the visual
  identity.
- **The renderer** (D3D9, atlas blitting, color modulation) works.

The rewrite should **reuse the widget implementations behind a better data and
binding layer**, not replace them wholesale. That is the core of the
recommendation in [`04-refactor-plan.md`](04-refactor-plan.md).

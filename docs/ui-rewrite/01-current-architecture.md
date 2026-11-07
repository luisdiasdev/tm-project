# 01 — How the UI Works Today

Everything here was read from the current tree on branch `test/748`. File paths
are relative to the repo root.

## 1. Layer map

```
             data files                         C++ runtime
  ┌──────────────────────────────┐   ┌────────────────────────────────────┐
  │ UI/*Scene*.bin   (layout)    │──▶│ TMScene::ReadRCBin  (TMScene.cpp)  │
  │ UI/UIString.txt  (label text)│──▶│ g_UIString[500][64]                │
  │ UI/strdef.bin    (messages)  │──▶│ g_pMessageStringTable[2000][128]   │
  │ UI/UITextureList.bin         │──▶│ TextureManager                     │
  │ UI/UITextureSetList.txt      │──▶│ ControlTextureSet[600]             │
  └──────────────────────────────┘   └──────────────┬─────────────────────┘
                                                     │ new SXxx(...)
                                                     ▼
                                   ┌────────────────────────────────────┐
                                   │ SControl tree (TreeNode)           │
                                   │   SControlContainer::m_pControlRoot│
                                   └───────┬──────────────────┬─────────┘
                                           │ input            │ FrameMove2
                                           ▼                  ▼
                                   ┌──────────────┐   ┌──────────────────┐
                                   │ FindControl  │   │ GeomControl lists│
                                   │ + OnControl  │   │ [MAX_DRAW=30]    │
                                   │   Event      │   └────────┬─────────┘
                                   └──────────────┘            ▼
                                                       RenderDevice::
                                                       RenderGeomControl
```

Core files:

| Area | File | Lines |
|---|---|---:|
| Control base + widgets | `Projects/TMProject/SControl.h/.cpp` | 670 / 3,051 |
| Container, input walk, draw list | `SControlContainer.h/.cpp` | 40 / 385 |
| Item grids (inventory, trade, skills) | `SGrid.h/.cpp` | 152 / 5,332 |
| Scene binary structs | `UIBinary.h` | 135 |
| Geometry record + draw list | `GeomObject.h/.cpp` | 49 / 73 |
| Text rasterizer | `TMFont2.h/.cpp`, `TMFont3.*` | 31+43 / 393+399 |
| Tree | `TreeNode.h/.cpp` | 54 / 224 |
| Scene base / loader | `TMScene.h/.cpp` | 147 / 2,698 |
| Field UI + gameplay | `TMFieldScene.h/.cpp` | 855 / 24,638 |
| Char select | `TMSelectCharScene.h/.cpp` | 106 / 2,237 |
| Server select | `TMSelectServerScene.h/.cpp` | 79 / 1,554 |

That is roughly **11.0k lines of UI engine** and **12k lines of scene UI code**
(~8.2k of the field file is strictly UI).

## 2. Data files

### 2.1 Scene binaries (`UI/FieldScene2.bin`, …)

Records are `int32 controlType` followed by a fixed-size body. The body always
starts with `nID, nParentID, nTextureSetIndex, nStartX, nStartY, nWidth,
nHeight`.

| Type | ID | Struct | Size |
|---|---:|---|---:|
| PANEL | 1 | `BinPanel` | 40 |
| BUTTON | 2 | `BinButton` | 40 |
| CHECKBOX | 3 | `BinCheckBox` | 32 |
| LISTBOX | 6 | `BinListBox` | 52 |
| PROGRESS | 10 | `BinProgress` | 48 |
| TEXT | 12 | `BinText` | 52 |
| EDIT | 13 | `BinEdit` | 184 |
| 3DOBJ | 15 | `Bin3DObj` | 28 |
| GRID | 16 | `BinGrid` | 40 |

There is **no header, version, name table, comment, or checksum**. Records have
no explicit z-order; order in the file *is* the order.

Actual 748 data:

| Scene | Bytes | Controls | Depth | Types |
|---|---:|---:|---:|---|
| `SelServerScene2.bin` | 1,352 | 22 | 1 | 5 button, 2 edit, 2 listbox, 7 panel, 6 text |
| `SelCharScene2.bin` | 4,820 | 85 | 1 | 11 button, 3 edit, 1 listbox, 17 panel, 53 text |
| `FieldScene2.bin` | 35,404 | 709 | 3 | 170 button, 7 edit, 125 grid, 28 listbox, 138 panel, 7 progress, 234 text |

`TMScene::LoadRC()` (`TMScene.cpp:291`) rewrites the requested `.txt` name to
`.bin` with a brittle last-7/last-3 character rule, then calls `ReadRCBin()`.
`ParseRC()` is an empty stub (`TMScene.cpp:310`) — the original text format path
is gone.

### 2.2 Texture sets

`UI/UITextureSetList.txt` is already a text format:

```
[SetName]
SetIndex: N
ItemCount: M
texIndex,startX,startY,width,height,destX,destY
```

748 has 446 sets, max index 456. Each set is a list of atlas frames. A control
references a set by `nTextureSetIndex`; `-1` means "text/no fill", `-2` means
"colored fill", and `< -2` means set `abs(value)`.

This is the *best-designed* part of the current system and should be kept.

### 2.3 Text and localization

There are **three** text pipelines:

1. **Scene labels**: `.bin` stores `nStringIndex`; the loader does
   `g_UIString[binTextData.nStringIndex]` (`TMScene.cpp:544`). `g_UIString` is
   `char[500][64]` (`TMGlobal.cpp:30`) filled by `ReadUIString()` with
   `sscanf("%d %s")` (`Basedef.cpp:1178`). Consequence: max 500 strings, 64
   bytes each, **single-word only**.
2. **Runtime messages**: `g_pMessageStringTable[2000][128]`, loaded from
   `UI/strdef.bin` (XOR `0x5A`, 4-byte checksum). Referenced by number from
   code, e.g. `g_pMessageStringTable[237]`.
3. **Hard-coded literals** in C++ (`"Confirm"`, `"Cancel"`, `"Question"`, …)
   and text files with inline color prefixes (`UI/interface.txt`,
   `UI/command.txt`, `UI/chardesc*.txt`).

So a visible string can live in three different places, and moving one between
them is a code change.

## 3. Control model

`SControl : TreeNode` holds position, size, visibility, focus, control ID, a
unique ID, and an `IEventListener*`. Widgets derive from it:

```
SControl
├── SPanel        (image / colored fill, drag-move, desc panel)
│   ├── SButton   (4 image states, alt text, blink)
│   ├── SCheckBox
│   ├── SProgressBar
│   ├── SListBox  (adds SScrollBar + SEditableText)
│   ├── SMessageBox
│   ├── SMessagePanel
│   ├── SReelPanel
│   ├── SGridControl (adds SGridControlItem : S3DObj)
│   └── SCursor
├── SText
│   └── SEditableText
├── S3DObj
├── SButtonBox
└── SScrollBar
```

`SControl::SControl` (`SControl.cpp:16`) multiplies **position and size** by
`screenWidth/800` and `screenHeight/600` at construction time. This is the only
systematic scaling in the whole UI.

The scene class keeps a raw pointer per bound control. `TMFieldScene.h` has
roughly **330 pointer members** — and several are arrays
(`m_pAffect[32]`, `m_pPartyAffectIcon[13][32]`, `m_pFireWorkButton[100]`, …),
so the number of pointer slots is higher.

## 4. Load path

`TMScene::ReadRCBin()` (`TMScene.cpp:315`) is a single 500-line `switch` over
the control type. For every record it:

1. reads the fixed body,
2. `new`s the matching widget,
3. calls `SetControlID(id)` and `SetCenterPos(id, x, y, w, h)`,
4. **looks up the parent by ID with `FindControl(parentID)`** and prepends the
   child,
5. attaches the container as the event listener for interactive widgets.

Consequences baked into the format:

- A parent record **must appear before its children**. The loader cannot
  forward-reference. The shipped files happen to satisfy this (0 violations in
  all three scenes), but nothing enforces it.
- `FindControl` is a linear tree walk (`SControlContainer.cpp:313`), so load
  time is O(n²) in the number of controls and parent lookups.
- Duplicate IDs silently make the later control unreachable by ID.
  `FieldScene2.bin` has two records with ID `6192`; `SelCharScene2.bin` has two
  with ID `631`.
- `SetCenterPos` (`SControl.cpp:223`) is a hard-coded list:

  ```cpp
  static unsigned int dwCenterUI[6] = { 769, 4622, 65870, 4617, 5638, 0 };
  for (...) if (dwControlID == dwCenterUI[i]) m_nPosX = screenW/2 - w/2;
  ```

  Only **X** is centered, and only for those five IDs. Centering is not a
  property of the data.

## 5. Tree, draw order, and z-index

`TreeNode::AddChild` **prepends** (`TreeNode.cpp:120`): the newest child becomes
`m_pDown`, which is traversed first. So among siblings, **the last record in the
file is drawn first (behind)**. That is why the 748 message box background must
be added last to sit behind its texts (`docs/port-748/message-box.md`).

Rendering is two-phase:

1. `SControlContainer::FrameMove` (`SControlContainer.cpp:259`) walks the tree
   depth-first. For each visible control it calls `FrameMove2(drawList, parentPos,
   layer, …)`. The widget appends its `GeomControl`(s) to
   `m_pDrawControl[layer]`.
2. `ObjectManager::RenderControl` (`ObjectManager.cpp:591`) iterates layers
   `0..29` and draws each list, then clears it.

Therefore:

- **Draw order = tree depth (layer) + reverse insertion order within a layer.**
- A child **always** draws after (on top of) its parent's own base. A child can
  never go behind the parent background.
- `MAX_DRAW_CONTROL` is **30** (`SControlContainer.h:7`). A control nested
  deeper than 29 is **silently dropped** (`AddRenderControlItem` returns 0).
- `m_bAlwaysOnTop` exists but is **never read by the renderer** — setting it has
  no effect.
- Modals are not a general concept. `SControlContainer` checks eight fixed
  slots, `m_pModalControl[8]`, and scenes fill specific indices by hand
  (`TMScene.cpp:200`, `TMFieldScene.cpp:1198-1216`, …).

## 6. Input path

`SControlContainer::OnMouseEvent` (`SControlContainer.cpp:30`) walks the same
tree, accumulating parent offsets, and calls each control's `OnMouseEvent` with
local coordinates. It records whether anything consumed the event (`bProcessed`)
but still visits the rest of the tree; there is no hit-test list, no capture
model, and no z-ordered picking beyond traversal order.

Widgets implement their own semantics:

- `SPanel::OnMouseEvent` handles drag-move, description panels, modal blocking,
  and contains hard-coded exceptions:
  `if (m_dwControlID == 65943 || m_dwControlID == 65947) return 0;`
  (`SControl.cpp:265`).
- `SGridControl::OnMouseEvent` (`SGrid.cpp`) implements item pickup, swap,
  sell, buy, trade, right-click, shift-click, drag between grids, and cursor
  attachment. It reaches into `TMFieldScene` and global state directly.
- `SEditableText` handles focus, IME, cursor, password masking, and key
  handling.

The cursor is a normal `SCursor` widget (`SControlContainer.cpp:14`) drawn at
layer 29.

## 7. Text rendering

`SText` owns **four** `TMFont2` instances (normal + three continuation chunks).
`TMFont2::SetText` (`TMFont2.cpp:28`):

1. caps the string at 120 characters,
2. splits it into up to three ~42-character chunks (because the glyph atlas is
   512 px wide),
3. rasterizes each chunk with **GDI `TextOut`** into a shared DIB
   (`RenderDevice::m_hDC`),
4. locks a D3D9 texture and copies the alpha into it,
5. `Render()` draws the chunks side by side.

So every `SetText` is a CPU rasterization + texture upload. Text is re-rendered
whenever a value changes; `UpdateScoreUI` and similar functions call `SetText`
on many labels, and `FrameMove` runs every frame. This is the single largest
performance and quality liability in the UI (blurry scaling, no real wrapping,
no Unicode beyond the code page, 120-char limit, chunk seams).

## 8. Scene lifecycle and runtime-built UI

Each scene's `InitializeScene()` does, in order:

1. `LoadRC("UI\\<Name>2.txt")` — parse the `.bin` into the tree,
2. create additional widgets in C++ (`new SPanel`, `new SButton`, …),
3. bind every control the code cares about with
   `m_pControlContainer->FindControl(ID)`,
4. apply per-control fixes: visibility, positions, sizes, colors, selection,
   texture sets, scrollbars.

`TMFieldScene::InitializeScene` is **1,632 lines**, with **375 `FindControl`
calls**, **104 `if (m_pX)` null guards**, and **61 `new` expressions**. The
whole field file has 511 `FindControl` calls, 273 of them numeric literals.

`TMScene::ReloadScene()` exists (`TMScene.cpp:1474`) but is **never called**.
There is no hot reload, no file watching, and no runtime scene rebuild.

Several panels are built entirely in code because they are not in the `.bin`:
the two message boxes, message panel, IME text, quiz panel, server window
(755/748 UI2), and many field popups. So the `.bin` is not the full UI truth.

## 9. Where behavior lives

`TMFieldScene::OnControlEvent` is **3,521 lines**. It is a chain of top-level
`if (idwControlID == …)` / range checks (139 branches), with 11 `switch`
statements and 56 `case` labels inside. Branches range from 1 line to 448
lines. It mixes:

- pure UI actions (open/close panel, select tab, set text),
- packet sends (`SendOneMessage`),
- game state mutation (`g_pObjectManager->m_stMobData`),
- sound, camera, chat parsing, item logic.

`TMSelectCharScene::OnControlEvent` (419 lines) and
`TMSelectServerScene::OnControlEvent` (424 lines) follow the same pattern.

The event is delivered as `(controlID, event)` from
`SControlContainer::OnControlEvent` → `TMScene::OnControlEvent` (pure virtual,
implemented per scene). There is no per-widget callback registration; every
widget funnels into the scene.

## 10. Summary of the design

| Concern | Today |
|---|---|
| Layout | fixed pixel records, scaled once at construction, per-control fixes in code |
| Style | texture set index + frame index + color + font flags per record |
| Binding | numeric control ID, linear `FindControl`, ~500 raw pointers per scene |
| Behavior | one giant `if/else` per scene, IDs compared inline |
| Text | GDI rasterization per change, 3 chunks, 120-char cap |
| Ordering | tree depth + reverse insertion, 30-layer cap |
| Modality | eight hand-assigned slots |
| Iteration | edit C++ → rebuild → relaunch → log in → navigate |
| Validation | none at runtime; a separate Python audit exists but is not enforced |

The next document turns each of these into a concrete pain point with a cost.

# 03 — Widget framework: classes, vtables, lifecycle

This document describes the recovered C++ object model behind `.pane`. All of
it comes from RTTI strings, constructors and virtual calls in `WYD.exe`.

## 1. Class hierarchy

`CGuiElement` is the base class. The constructors tell us the inheritance chain
(the first `FUN_*` call in each constructor is the parent constructor):

```text
CGuiElement
├── CGuiStatic
│   ├── CGuiButton
│   │   ├── CGuiCheckBox
│   │   │   └── CGuiRadioButton
│   │   ├── CGuiComboBox
│   │   └── CGuiIconButton
│   ├── CGuiGaugeBar
│   └── CBalloonBox
│       └── CTooltipBox
├── CGuiImage
├── CGuiSlider
├── CGuiEditBox
│   └── CGuiImeEditBox
│       └── CGuiImeNotePadEditBox
├── CGuiListBox
│   └── CGuiListView
│       └── CGuiTreeView
├── CGuiImageFont
├── CGuiImeNotePad
├── CGuiMesh
└── CGuiCustomListView
```

RTTI names confirmed in the binary:

```text
.?AVCGuiElement@@      .?AVCGuiStatic@@       .?AVCGuiButton@@
.?AVCGuiCheckBox@@     .?AVCGuiRadioButton@@  .?AVCGuiComboBox@@
.?AVCGuiIconButton@@   .?AVCGuiGaugeBar@@     .?AVCBalloonBox@@
.?AVCTooltipBox@@      .?AVCGuiImage@@        .?AVCGuiSlider@@
.?AVCGuiEditBox@@      .?AVCGuiImeEditBox@@   .?AVCGuiImeNotePad@@
.?AVCGuiImeNotePadEditBox@@                   .?AVCGuiListBox@@
.?AVCGuiListView@@     .?AVCGuiTreeView@@     .?AVCGuiImageFont@@
.?AVCGuiMesh@@         .?AVCGuiCustomListView@@
.?AVCGuiPanel@@        .?AVCGuiContents@@      .?AVCGuiAnimation@@
```

The panel side is `CGuiPanel`, with these related classes:

```text
CGuiContents
CGuiPanel
CGuiPage
CGuiPageGroup
CGuiAnimation / CGuiAnimationScript
CGuiFadeAnimation / CGuiSlideAnimation / CGuiScaleAnimation
CGuiAnimationManager
```

## 2. `CGuiElement` vtable map

The base vtable is at `CGuiElement::vftable` = `0x88041c`. Derived classes append
their own virtuals after the base entries.

| Slot | Base function | Meaning |
|---:|---|---|
| `0x00` | `0x4478d0` | scalar deleting destructor |
| `0x04` | `0x436010` | on-name-set / on-create hook; `Button` parses `ok`/`cancel` here |
| `0x08` | `0x42e9f0` | post-load/default-population hook; `EditBox` sets sample text, `ListBox` can add test items |
| `0x0c` | `0x4476d0` | **draw** (base is empty) |
| `0x10` | `0x436050` | draw overlay/secondary pass (base empty) |
| `0x14` | `0x42ea00` | finalize/return-false hook |
| `0x18` | `0x436060` | **on key** (returns handled) |
| `0x1c` | `0x436070` | **on mouse event** (returns handled) |
| `0x20` | `0x436080` | can-focus / can-interact |
| `0x24` | `0x42ea10` | focus gained |
| `0x28` | `0x42ea20` | focus lost |
| `0x2c` | `0x442a30` | mouse enter (`+9 = 1`, calls `+0x74`) |
| `0x30` | `0x442a40` | mouse leave (`+9 = 0`, calls `+0x78`) |
| `0x34` | `0x436090` | activate / click |
| `0x38` | `0x42ea30` | event-bubble hook (returns false) |
| `0x3c` | `0x43f610` | **hit test** (`PtInRect` on `+0xbc`) |
| `0x40` | `0x43f8b0` | clear/reset text |
| `0x44` | `0x42ea40` | set enabled (`+0xcc`) |
| `0x48` | `0x42ea50` | get enabled |
| `0x4c` | `0x42ea60` | set visible (`+0x8`) |
| `0x50` | `0x42ea70` | get visible |
| `0x54` | `0x444250` | per-frame tick: advances animation objects |
| `0x58` | `0x4441e0` | release slot/font resources |
| `0x5c` | `0x442e60` | set texture color (packed ARGB → float RGBA) |
| `0x60` | `0x442d60` | **extra property handler** (base no-op) |
| `0x64` | `0x444360` | copy/clone state from another element |
| `0x68` | `0x42ea80` | resize/event hook (base no-op) |
| `0x6c` | `0x445610` | **serialize to text** (writes `.pane` keys) |
| `0x70` | `0x442f80` | recompute absolute rect `+0xbc` from `+0x14..+0x20` |
| `0x74` | derived-only | tooltip show |
| `0x78` | derived-only | tooltip hide |
| `0x7c` | Button-only | activate/click event |
| `0x80` | Button-only | toggle callback |

The draw override per class is the `0x0c` entry:

| Class | Draw function |
|---|---|
| `CGuiStatic` | `0x48a130` |
| `CGuiImage` | `0x449c10` |
| `CGuiButton` | `0x436520` |
| `CGuiCheckBox` / `CGuiRadioButton` | `0x437a20` |
| `CGuiGaugeBar` | `0x448900` |
| `CGuiComboBox` | `0x437ee0` |
| `CGuiSlider` | `0x4899b0` |
| `CGuiEditBox` / `CGuiImeEditBox` | `0x441e20` |
| `CGuiListBox` | `0x4564e0` |
| `CGuiListView` | `0x458840` |
| `CGuiImageFont` | `0x44baf0` |
| `CGuiIconButton` | `0x42c550` |
| `CGuiMesh` | `0x46d480` |
| `CGuiCustomListView` | `0x43ba20` |

## 3. Core geometry model

The base element stores a **local** rectangle at `+0x14`:

```text
+0x14 left
+0x18 top
+0x1c width
+0x20 height
```

The update function `FUN_00442f80` (`0x442f80`, vtable `+0x70`) computes the
**absolute** rectangle at `+0xbc`:

```c
SetRect(absolute, left, top, left + width, top + height);
```

When the parent panel moves or the screen resolution changes, the absolute rect
is recomputed. Hit testing (`0x3c`) always uses the absolute rect.

The parser's `Area` key sets `left top width height`. `Rect` sets corners and
converts to width/height. `Left`/`Top`/`Width`/`Height` modify one member and
recompute the rectangle.

## 4. Common state flags

| Offset | Meaning |
|---|---|
| `+0x08` | visible |
| `+0x09` | mouse-over/hover |
| `+0x0a` | focus |
| `+0x0b` | default element |
| `+0x0e` | BelowPanel |
| `+0x10` | MsgReceive |
| `+0x24` | parent panel pointer |
| `+0x28` | font/renderer object |
| `+0x2c` | second font/animation object |
| `+0x30` | vector of texture-slot objects (`begin`) |
| `+0x34` | vector of texture-slot objects (`end`) |
| `+0x40` | shadow type |
| `+0x44` | shadow color (packed ARGB) |
| `+0x50` | element id |
| `+0x58` | PageIndex |
| `+0x64` | name `std::wstring` (data/length/capacity follow the MSVC layout) |
| `+0xbc` | absolute `RECT` |
| `+0xcc` | enabled |

## 5. Texture slots and state colors

Each element owns a vector of texture-slot objects at `+0x30`/`+0x34`. The
vector length determines how many `TextureAlias[n]` indices are valid.

Each slot object has:

| Offset | Meaning |
|---|---|
| `+0x00` | texture alias `std::wstring` |
| `+0x14` | font index |
| `+0x18` | font-align flags |
| `+0x1c`/`+0x20` | font offset x/y |
| `+0x24` | texture color `Normal` |
| `+0x28` | texture color `Pressed` |
| `+0x2c` | texture color `MouseOver` |
| `+0x30` | texture color `Disabled` |
| `+0x34..0x40` | float RGBA tint used for drawing |
| `+0x44` | font color `Normal` |
| `+0x48` | font color `Pressed` |
| `+0x4c` | font color `MouseOver` |
| `+0x50` | font color `Disabled` |
| `+0x54..0x60` | float RGBA font tint |
| `+0x64` | raster/draw mode (0–4) |

This matches the `.pane` keys:

```text
RasterOp[n]
TextureAlias[n]
TextureColor.Normal[n]
TextureColor.Pressed[n]
TextureColor.MouseOver[n]
TextureColor.Disabled[n]
Font[n]
FontAlign[n]
FontOffset[n]
FontColor[n]
FontColor.Normal[n] / .Pressed[n] / .MouseOver[n] / .Disabled[n]
```

The state enum used by the drawing code is:

```text
0 = normal
1 = pressed
2 = mouse-over
3 = disabled
```

## 6. Draw pipeline

Every class draws through the shared renderer:

```text
FUN_00477820(texture, drawMode, destRect, sourceRect)   // textured quad
FUN_00477710(texture, destRect)                          // simple quad
FUN_004777b0(texture, centerX, centerY, rotX, rotY, scale, angle) // rotated
FUN_00477cd0(text, font, destRect, count, shadowType, shadowColor, ...)
FUN_0047b7d0(font, text, color, rect, count, shadowType, shadowColor, ...)
FUN_00498860(font, text, x, y, ...)                      // multiline text
```

`RasterOp` maps to the draw mode:

```text
0 wrap
1 stretch
2 split          (9-slice: corners/edges/center)
3 wSplitStretch
4 hStretchSplit
```

`split` is the common 9-slice mode used by windows, buttons and frames.

## 7. Event model

The UI event dispatcher calls:

```text
element->vtable[0x1c](message, wParam, lParam, ...)   // OnMouseEvent
element->vtable[0x18](message, key)                    // OnKey
element->vtable[0x3c](x, y)                            // HitTest
element->vtable[0x34]()                                // Activate
```

Input handling is **immediate-mode-ish**:

- The framework tracks the pressed element globally.
- `Button::OnMouseEvent` handles `WM_LBUTTONDOWN`/`UP`/`DBLCLK`, captures the
  mouse, sets `+0x211` while pressed, and calls the event system on release.
- `EditBox::OnMouseEvent` handles caret placement, selection and IME.
- `Slider::OnMouseEvent` drags the knob and clamps to `range`.
- `ListBox`/`ListView` handle selection, wheel, scrollbar drag and keyboard
  navigation.

Event ids observed in the code:

```text
0x101  click / command
0x111  alternate click path
0x115  cancel / right-click-ish path
0x601  editbox enter
0x603  editbox focus gained
0x604  editbox focus lost
0x608  editbox tab
0x609  editbox page up
0x610  editbox page down
```

`Button` also parses its instance name at load time: an element named `ok`
sets `+0x214`, and one named `cancel` sets `+0x215`. Those flags make Enter/Esc
activate the button.

## 8. Panel-specific behaviour

`CGuiPanel` has its own vtable at `CGuiPanel::vftable` = `0x882880`.

- `+0x20` is the property setter (`FUN_0047df30`).
- The panel creates/owns child elements and a z-order.
- `Resource` loads a `.guimat` and resolves texture aliases/fonts.
- `AlignPanelX/Y`, `OffsetX/Y`, `ScreenAlign` position the panel.
- `Titlebar`, `Caption*`, `CloseButton*` draw the window chrome.
- `Animation` attaches a script (fade/slide/scale) to the panel.
- `ListView.itemPanel` loads a child `.pane` as a row template via
  `FUN_00481a70`.

## 9. Object lifetime

1. Parser reads `Panel` → creates/uses a `CGuiPanel`.
2. Panel property lines call vtable `+0x20`.
3. Parser reads `element Type(name)` → factory `FUN_00447900` constructs the
   concrete class and attaches it to the panel.
4. Element property lines call the common setter `FUN_00444590`, which forwards
   unknown keys to vtable `+0x60`.
5. At the end of an element block the parser calls vtable `+0x08` when the
   global "post-load" flag is set.
6. At the end of the file the panel gets its lifecycle hooks (`+0x14`, `+0x10`)
   and the first layout pass.
7. Each frame: panel/element `+0x54` tick, then `+0x0c` draw.

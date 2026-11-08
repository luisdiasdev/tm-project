# Recovered class hierarchy and layout

## 1. Hierarchy

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

CGuiPanel
├── CGuiPage
└── CGuiPageGroup

CGuiContents
CGuiAnimation
├── CGuiAnimationScript
├── CGuiFadeAnimation
├── CGuiSlideAnimation
└── CGuiScaleAnimation
CGuiAnimationManager
```

The inheritance lines are proven by the first parent-constructor call in each
constructor.

## 2. RTTI strings

The `.data` RTTI block at `0x954000` contains the type names:

```text
.?AVCGuiElement@@
.?AVCGuiStatic@@
.?AVCGuiButton@@
.?AVCGuiCheckBox@@
.?AVCGuiRadioButton@@
.?AVCGuiGaugeBar@@
.?AVCGuiComboBox@@
.?AVCGuiSlider@@
.?AVCGuiEditBox@@
.?AVCGuiImeEditBox@@
.?AVCGuiImeNotePad@@
.?AVCGuiImeNotePadEditBox@@
.?AVCGuiListBox@@
.?AVCGuiListView@@
.?AVCGuiTreeView@@
.?AVCGuiImage@@
.?AVCGuiImageFont@@
.?AVCGuiIconButton@@
.?AVCGuiMesh@@
.?AVCGuiCustomListView@@
.?AVCGuiPanel@@
.?AVCGuiContents@@
.?AVCGuiAnimation@@
.?AVCGuiAnimationScript@@
.?AVCGuiFadeAnimation@@
.?AVCGuiSlideAnimation@@
.?AVCGuiScaleAnimation@@
.?AVCGuiAnimationManager@@
.?AVCBalloonBox@@
.?AVCTooltipBox@@
```

## 3. `CGuiElement` field layout (selected)

Offsets are byte offsets from the object base. They are recovered from the
property setters, the draw functions and the copy/clone function.

| Offset | Type | Meaning |
|---:|---|---|
| `0x00` | vtable | object vtable |
| `0x04` | ptr | parent/owning panel or manager |
| `0x08` | bool | visible |
| `0x09` | bool | mouse-over |
| `0x0a` | bool | focus |
| `0x0b` | bool | default element |
| `0x0e` | bool | BelowPanel |
| `0x10` | bool | MsgReceive |
| `0x14` | int | left |
| `0x18` | int | top |
| `0x1c` | int | width |
| `0x20` | int | height |
| `0x24` | ptr | parent panel |
| `0x28` | ptr | font/text object |
| `0x2c` | ptr | second font/animation object |
| `0x30` | ptr | texture-slot vector begin |
| `0x34` | ptr | texture-slot vector end |
| `0x38` | ptr | texture-slot vector capacity |
| `0x40` | int | shadow type |
| `0x44` | uint | shadow color (ARGB) |
| `0x50` | int | element id |
| `0x58` | uchar | PageIndex |
| `0x64` | wstring | name |
| `0x8c` | wstring | text/caption buffer (widget-dependent) |
| `0xbc` | RECT | absolute rect (int32 x4) |
| `0xcc` | bool | enabled |

### `CGuiStatic`

| Offset | Type | Meaning |
|---:|---|---|
| `0x1b8` | wstring | caption/text |
| `0x1f8` | int | MultiLineGap |
| `0x1fc` | int | FontOffset x |
| `0x200` | int | FontOffset y |
| `0x20c` | bool | MultiLine |
| `0x20d` | bool | RichText |

### `CGuiImage`

| Offset | Type | Meaning |
|---:|---|---|
| `0x180` | float | animation elapsed |
| `0x184` | uchar | current frame |
| `0x185` | uchar | loop counter |
| `0x186` | bool | UseRotation |
| `0x198` | bool | UseAni |
| `0x199` | uchar | AniCutXPage |
| `0x19a` | uchar | AniCutYPage |
| `0x19b` | uchar | AnimationMinPage |
| `0x19c` | uchar | AnimationMaxPage |
| `0x1a0` | float | AnimationDelay |
| `0x1a4` | short | AnimationLoopCnt |
| `0x1a6` | bool | external-image animation flag |
| `0x1a8` | float | Rotation (degrees) |
| `0x1ac` | int | RotationCenter x |
| `0x1b0` | int | RotationCenter y |
| `0x1b4` | ptr | external image object |

### `CGuiButton`

| Offset | Type | Meaning |
|---:|---|---|
| `0x211` | bool | pressed |
| `0x213` | bool | ? |
| `0x214` | bool | `ok` button flag |
| `0x215` | bool | `cancel` button flag |
| `0x216` | bool | ToggleButton |
| `0x218` | int | icon size/offset |
| `0x21c` | bool | bounce enabled |
| `0x220` | float | bounce timer |
| `0x295` | bool | DownTextUp |
| `0x298` | int | ButtonSound |

### `CGuiCheckBox`

| Offset | Type | Meaning |
|---:|---|---|
| `0x2a0` | bool | checked |
| `0x2a4` | RECT | check-mark rect |
| `0x2b4` | RECT | text rect |

### `CGuiGaugeBar`

| Offset | Type | Meaning |
|---:|---|---|
| `0x210` | int | range minimum |
| `0x214` | int | range maximum |
| `0x218` | int | current position |
| `0x220` | int | BarOutSpace left |
| `0x224` | int | BarOutSpace top |
| `0x228` | int | BarOutSpace right |
| `0x22c` | int | BarOutSpace bottom |
| `0x250` | int | text type (0 none, 1 fraction, 2 percent) |
| `0x254` | int | BarDirection (0 right, 1 left, 2 up, 3 down) |
| `0x290` | bool | noFrame |
| `0x291` | bool | BarFlash active |
| `0x292` | bool | hide when flash ends |
| `0x294..0x2a0` | RECT | BarSpark rect |
| `0x2a4..0x2b0` | RECT | BarFlash rect |
| `0x2b4..0x2c4` | wstring | BarSpark alias |
| `0x2c8..0x2d8` | wstring | BarFlash alias |
| `0x2e4` | int | FillDrawMethod |
| `0x2ec` | float | BarFlashFadeoutRate |

### `CGuiEditBox`

| Offset | Type | Meaning |
|---:|---|---|
| `0x180` | wstring | text |
| `0x194` | int | spacing |
| `0x198..0x1a4` | RECT | text/client rect |
| `0x1a8..0x1b4` | RECT | box rect |
| `0x248` | int | caret position |
| `0x254` | int | selection anchor/start |
| `0x25c` | uint | font color |
| `0x260` | uint | SelColor |
| `0x264` | uint | SelBkColor |
| `0x268` | uint | CaretColor |
| `0x26c` | int | Range (max length) |
| `0x274` | bool | NumAttrib |
| `0x275` | bool | DrawBox |
| `0x2cc` | wstring | placeholder/empty text |
| `0x3d8` | bool | box visible flag |
| `0x3d9` | bool | Password |

### `CGuiListBox`

| Offset | Type | Meaning |
|---:|---|---|
| `0x37c` | int | first visible item index |
| `0x3b0` | flags | selection flags |
| `0x3b4` | int | selected index |
| `0x3b8` | int | anchor index |
| `0x3bc` | bool | shift-selection mode |
| `0x3c0` | ptr | item vector begin |
| `0x3c4` | ptr | item vector end |
| `0x18d` | bool | RichText |
| `0x195` | bool | DrawSelectBoxLast |

### `CGuiListView` / `CGuiCustomListView`

| Offset | Type | Meaning |
|---:|---|---|
| `0x3d0` | int | item width |
| `0x3d4` | int | item height |
| `0x3d8` | wstring | item panel path |
| `0x3e0` | ptr | item panel instance/manager |

## 4. Vtable layout

The base `CGuiElement` vtable is 29 entries (`0x00..0x70`). Derived classes
append extra entries:

- `CGuiImage`: `0x74` tooltip show, `0x78` tooltip hide
- `CGuiStatic`: `0x74` tooltip show, `0x78` tooltip hide
- `CGuiButton`: inherits Static `0x74/0x78`, adds `0x7c` activate/click and
  `0x80` toggle callback
- `CGuiListBox`: `0x74..` list-specific methods
- `CGuiListView`: more item/scroll methods

See [`../03-widget-framework.md`](../03-widget-framework.md) for the full
slot-by-slot table.

## 5. Copy/clone behaviour

`FUN_00444360` (vtable `+0x64`) copies from another element:

- local rect (`+0x14..+0x20`)
- visibility, MsgReceive
- name (`+0x64`)
- text (`+0x8c` / widget-specific)
- slot vector: frees old slots, allocates new ones, copies all slot fields
- calls `+0x70` to recompute absolute rect

This is useful as a field inventory when implementing clone/duplicate.

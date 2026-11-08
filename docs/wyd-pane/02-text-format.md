# 02 — Text format and property reference

## 1. Encoding and line grammar

After decryption the file is UTF-16LE with a `FF FE` BOM and CRLF line endings.

The parser is line-oriented:

```text
Panel
{
    <key> <value>
    ...
}

element <Type>(<name>)
{
    <key> <value>
    ...
}
```

Rules:

- Keys are **case-insensitive** (`__wcsicmp`).
- The key is the **first whitespace-delimited token**. The value is **the rest
  of the line**, with leading/trailing whitespace trimmed. Values may therefore
  contain spaces, e.g. `Text Copyright 2002~2021 Joyimpact All Rights Reserved.`
- Numeric lists are re-tokenized on whitespace.
- Booleans are `true` / `false`.
- Colors are `A R G B`, packed as `0xAARRGGBB`. The first component is alpha.
- A line whose trimmed text is `}` closes the current block.
- Lines beginning with `//` are comments and are ignored, including before
  `Panel` (see `GameConfig.pane`).
- Unknown keys are silently ignored.
- `element <Type>` may omit the instance name.
- The `Panel` block should come first. Elements are appended to that panel.
- In practice each file has exactly one `Panel` block.

The tokenizer uses the delimiter set `"\t ()"` for element headers, so
`element Image ( bg )` and `element Image(bg)` are equivalent.

## 2. `Panel` attributes

Parsed by `FUN_0047df30` (`0x47df30`), the `CGuiPanel` virtual property setter
(vtable slot `+0x20`).

| Key | Value | Effect |
|---|---|---|
| `Resource` | path | Loads the `.guimat` used for aliases/fonts. All shipped panels use `ui/default.guimat`. |
| `AlignPanelX` | `NONE` / `Center` / `SIDE` | Horizontal screen anchor. Stored as 0/1/2. |
| `AlignPanelY` | `NONE` / `Center` / `SIDE` | Vertical screen anchor. Stored as 0/1/2. |
| `OffsetX` | float | Offset after anchoring. |
| `OffsetY` | float | Offset after anchoring. |
| `ScreenAlign` | `left top right bottom …` | Bit flags: left=1, top=2, right=4, bottom=8. Multiple tokens allowed. |
| `Area` | `x y w h` | Sets the panel rect from position and size. |
| `Rect` | `l t r b` | Sets the panel rect from corners. |
| `Left` / `Top` | int | Position. |
| `Width` / `Height` | int | Size. |
| `ZOrder` | int | Draw order. Calls `FUN_00479f40`. |
| `Color` or `TextureColor` | `A R G B` | Panel tint. Stored at `+0x28`. |
| `TextureAlias` | alias | Panel background alias. |
| `TextureOffset` | `x y w h` | Texture offset/scroll rect. |
| `RasterOp` | `wrap` / `stretch` / `split` / `wSplitStretch` / `hStretchSplit` | Draw mode 0–4. |
| `Titlebar` | `true` / `false` | Show/hide the title bar. |
| `TitlebarHeight` | int | Title bar height. |
| `Caption` | string | Title text; may be a `uu_*` localization key. |
| `CaptionFont` | int | Index into the guimat `FontList`. |
| `CaptionColor` | `A R G B` | Title color. |
| `CaptionShadowType` | `None` / `Outline` / `BottomRight` / `Top` | Title shadow mode. |
| `CaptionShadowColor` | `A R G B` | Title shadow color. |
| `CloseButton` | `true` | Creates the close button. |
| `CloseButtonLocation` | `x y w h` | Close button rect. |
| `AllowFocus` | `false` | Disables keyboard focus. |
| `PageIndex` | int | Page index for page-group panels. |
| `OpenSound` | int | Sound id on open. |
| `CloseSound` | int | Sound id on close. |
| `resizeTip` | flags | top=1, bottom=2, left=4, right=8, topRight=0x10, topLeft=0x20, bottomRight=0x40, bottomLeft=0x80, all=-1. |
| `PanelScaleType` | int | 0 = normal; nonzero enables panel scaling. |
| `Animation` | inline JSON | Animation script passed to the animation manager. |

### Panel example (`notice.pane`)

```text
Panel
{
    Resource ui/default.guimat
    AlignPanelX Center
    AlignPanelY NONE
    OffsetX 0.000000
    OffsetY 58.000000
    Width 500
    Height 25
    Caption 
    CaptionColor 255 255 225 120
    CaptionShadowType Outline
    CaptionShadowColor 255 0 0 0
    TextureAlias 
    RasterOp split
    Color 0 255 255 255
    ZOrder 200
    Animation 
}
```

### Animation JSON

Observed schema:

```json
[
  {
    "Name": "Open",
    "ScriptOne": [
      { "Time": 0, "Type": 3, "Target": "Panel",
        "nParam0": 100, "fParam0": 0, "fParam1": 1 }
    ]
  },
  {
    "Name": "Close",
    "ScriptOne": [
      { "Time": 0, "Type": 3, "Target": "Panel",
        "nParam0": 100, "fParam0": 1, "fParam1": 0 },
      { "Time": 100, "Type": 1, "Target": "Panel", "nParam0": 0 }
    ]
  }
]
```

The `.pane` loader does not interpret this itself; it calls the animation
manager (`FUN_004340d0`). Keep the value on one line: the block parser is
line-oriented and would treat a brace on its own line as a block terminator.

## 3. Element header

```text
element <Type>
element <Type>(<name>)
```

The factory is `FUN_00447900` (`0x447900`). Type names are case-insensitive.
`<name>` is the instance identifier used by code/scripts; it is not a filename.

Accepted types:

```text
Static
Image
Button
CheckBox
RadioButton
GaugeBar
ComboBox
Slider
EditBox
ImeEditBox
ListBox
ListView
BalloonBox
TooltipBox
ImageFont
TreeView
ImeNotePad
ImeNotePadEditBox
IconButton
MoMesh
CustomListView
```

Observed in shipped files:

```text
Static, Image, Button, CheckBox, GaugeBar, EditBox, ImeEditBox,
ImeNotePad, ListView, IconButton
```

## 4. Common element attributes

Parsed by `FUN_00444590` (`0x444590`). Unknown keys are forwarded to the
per-type handler at vtable `+0x60`.

| Key | Value | Effect |
|---|---|---|
| `id` | int | Explicit element id; otherwise the panel assigns an incremented id. |
| `name` | string | Element instance name (normally set by the header). |
| `PageIndex` | int | Page index. |
| `enabled` | `true`/`false` | Enabled flag (`+0xcc`). |
| `visible` | `true`/`false` | Visibility (`+0x8`). |
| `MsgReceive` | `true`/`false` | Receive mouse messages (`+0x10`). |
| `default` | `true`/`false` | Marks the element as the default (anything except `false` enables). |
| `BelowPanel` | `true` | Render below the panel. |
| `Area` | `l t w h` | Rect: left, top, **width**, **height**. |
| `Rect` | `l t r b` | Rect by corners; converted to width/height. |
| `Left` / `Top` / `Width` / `Height` | int | Individual rect members. |
| `ShadowType` | `None` / `Outline` / `BottomRight` / `Top` | Text shadow mode. |
| `ShadowColor` | `A R G B` | Text shadow color. |
| `RasterOp` | `wrap`/`stretch`/`split`/`wSplitStretch`/`hStretchSplit` | Applies to all texture slots. |
| `RasterOp[n]` | same | Per-slot draw mode. |
| `Font` | int 0..20 | Font index for all slots. |
| `Font[n]` | int 0..20 | Font index for slot `n`. |
| `FontAlign` | `left`/`center`/`right`, `top`/`vcenter`/`bottom`, `wordbreak` | Text alignment for all slots. |
| `FontAlign[n]` | same | Per-slot alignment. |
| `FontOffset` | `x y` | Text offset for all slots. |
| `FontOffset[n]` | `x y` | Per-slot text offset. |
| `FontColor` | `A R G B` | Text color for all slots. |
| `FontColor[n]` | `A R G B` | Per-slot text color. |
| `FontColor.Normal[n]` | `A R G B` | Per-state text color. |
| `FontColor.Pressed[n]` | `A R G B` | Per-state text color. |
| `FontColor.MouseOver[n]` | `A R G B` | Per-state text color. |
| `FontColor.Disabled[n]` | `A R G B` | Per-state text color. |
| `TextureAlias` | alias | Sets slot 0; also propagated to all slots when no per-slot mode is active. |
| `TextureAlias[n]` | alias | Per-slot texture alias. |
| `TextureColor` | `A R G B` | Tint for all slots. |
| `TextureColor[n]` | `A R G B` | Per-slot tint. |
| `TextureColor.Normal[n]` | `A R G B` | Per-state tint. |
| `TextureColor.Pressed[n]` | `A R G B` | Per-state tint. |
| `TextureColor.MouseOver[n]` | `A R G B` | Per-state tint. |
| `TextureColor.Disabled[n]` | `A R G B` | Per-state tint. |
| `Tagname` | string | Scripting tag. |

Texture slot indices are 0-based. Whatever slot count the element allocates,
the parser ignores `[n]` when `n` is out of range.

## 5. Per-type attributes

| Type | Extra keys |
|---|---|
| `Static` | `Caption`, `Text`, `ToolTip`, `MultiLine` (bool), `MultiLineGap` (int, default 5), `RichText` (bool) |
| `Button` | `Group`, `ToggleButton` (bool), `Icon`, `DownTextUp` (bool), `ButtonSound` (int); falls back to `Static` |
| `CheckBox` | same handler as `Button` |
| `RadioButton` | delegates to `Button` |
| `ComboBox` | `dropbox_length` (int); falls back to `Button` |
| `IconButton` | `IconTexture`, `IconTextureSlot` (int), `IconSlotIndex` (int), `IconSlotType` (int); falls back to `Button` |
| `Image` | `UseRotation` (bool), `Rotation` (float), `RotationCenter` (`x y`), `ExternalImageFile` (path), `UseAni` (bool), `AniCutXPage` (int), `AniCutYPage` (int), `AnimationMinPage` (int), `AnimationMaxPage` (int), `AnimationDelay` (float), `AnimationLoopCnt` (int, -1 = infinite), `ToolTip` |
| `GaugeBar` | `BarOutSpace` (`l t r b`), `noFrame` (bool), `text_type` (`rate_fraction`/`rate_percent`), `BarRange` (`min max`), `BarPos` (int), `text_postfix` (string), `BarDirection` (`right`/`left`/`up`/`down`), `BarSpark` (`alias x y w h`), `BarFlash` (`alias x y w h`), `BarFlashFadeoutRate` (float), `FillDrawMethod` (int); falls back to `Static` |
| `Slider` | `range` (`min max`), `value` (int), `MouseMoveLock` (bool) |
| `EditBox` | `SelBkColor` (`A R G B`), `SelColor` (`A R G B`), `CaretColor` (`A R G B`), `BorderWidth` (int), `Spacing` (int), `multiline` (bool), `indicator` (bool), `NumAttrib` (bool), `Range` (int), `DrawBox` (bool), `Password` (bool) |
| `ImeEditBox` | same as `EditBox` |
| `ImeNotePadEditBox` | same as `EditBox` |
| `ListBox` | `leftScrollbar` (bool), `ScrollBarWidth` (int), `borderWidth` (int), `spacing` (int), `itemSpacing` (int), `RichText` (bool), `DrawSelectBoxLast` (bool), `RenderMethod` (int) |
| `ListView` | all `ListBox` keys, plus `itemSize` (`w h`) and `itemPanel` (path to another `.pane` row template) |
| `TreeView` | delegates to `ListView` |
| `CustomListView` | all `ListBox` keys, plus `itemSize` and `itemPanel` |
| `ImeNotePad` | `leftScrollbar` (bool), `TestString` (string), `borderWidth` (int), `spacing` (int), `itemSpacing` (int), `RichText` (bool), `Wrap` (bool) |
| `ImageFont` | `FontHeight` (int), `Margin` (`x y`), `WidthScale` (float), `FontPreAlise` (string), `TextValue` (string) |
| `MoMesh` | `MoFile` (string), `Thickness` (int), `ParticleSize` (int) |
| `BalloonBox` | inherits `Static` |
| `TooltipBox` | inherits `Static` |

## 6. Representative elements

```text
element Image(bg)
{
    Area 0 0 500 25
    MsgReceive false
    RasterOp split
    TextureAlias UI.NoticeBG
    TextureColor 150 255 255 255
}

element Static(msg)
{
    Area 0 0 500 25
    MsgReceive false
    ShadowType BottomRight
    FontAlign center vcenter
    Caption uu_???  ...
}

element Button(cancel)
{
    Area 147 136 78 17
    Caption uu_210
}
```

An animated image:

```text
element Image(ExpLamp0)
{
    Area 0 0 16 16
    UseAni true
    AniCutXPage 8
    AniCutYPage 1
    AnimationMinPage 0
    AnimationMaxPage 7
    AnimationDelay 0.08
    AnimationLoopCnt -1
    TextureAlias ...
}
```

A list with a row template:

```text
element ListView(List)
{
    Area 0 0 300 200
    itemSize 300 17
    itemPanel ui/Panel/game/chatItem.pane
    ScrollBarWidth 12
    leftScrollbar true
}
```

## 7. Ordering, ids and names

- Elements are appended to the panel in file order.
- `FUN_0047c640` increments the panel's child counter (`+0x718`) and assigns it
  to the element's `+0x50`; an explicit `id` key overrides it.
- The `element Type(name)` string is stored as the element name. Several panels
  rely on that name for script lookups.
- `Tagname` is a separate string used by the UI event/script layer.

## 8. Parser entry points

| Address | Function | Role |
|---|---|---|
| `0x4825b0` | `FUN_004825b0` | top-level line loop, `Panel` and `element` dispatch |
| `0x481fd0` | `FUN_00481fd0` | `Panel { … }` body |
| `0x4817f0` | `FUN_004817f0` | `element { … }` body |
| `0x47df30` | `FUN_0047df30` | panel property setter |
| `0x444590` | `FUN_00444590` | common element property setter |

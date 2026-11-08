# 04 — Widget reference and observed behaviour

This is a behaviour guide derived from the draw/input functions in `WYD.exe`.
Function addresses are provided so each claim can be checked in the pseudocode
dumps.

Common conventions:

- `visible` and `enabled` are checked before draw/input.
- `Area` is `left top width height`; the absolute rect is recomputed into
  `+0xbc`.
- Texture states are `0 normal`, `1 pressed`, `2 mouse-over`, `3 disabled`.
- `RasterOp` values: `0 wrap`, `1 stretch`, `2 split` (9-slice),
  `3 wSplitStretch`, `4 hStretchSplit`.
- Colors are `A R G B`.
- `FontAlign` accepts horizontal (`left`/`center`/`right`), vertical
  (`top`/`vcenter`/`bottom`) and `wordbreak`.

---

## `Static` — `0x48a130`

Text-only element (also the base of buttons, labels, gauge captions, etc.).

- If `MultiLine` is false, draws one line with `FUN_0047b7d0`.
- If `MultiLine` is true, draws wrapped text with `FUN_00498860`.
- Uses `FontOffset` for a small x/y nudge.
- Uses `FontAlign` to position text in the rect.
- Uses `ShadowType` / `ShadowColor` for outline/bottom-right/top shadows.
- `RichText` enables inline markup interpretation by the text renderer.
- `MultiLineGap` is the extra line spacing (default 5).
- `Caption` and `Text` both set the text; `Caption` is the usual label setter,
  `Text` the body setter.
- `ToolTip` is shown by the mouse-enter hook (`+0x74`).

---

## `Image` — `0x449c10`

Draws texture slot 0 or an external image.

- Normal case: draws slot 0 into the element rect with the slot's `RasterOp`.
- `UseAni true`:
  - The source texture is treated as a sprite sheet.
  - `AniCutXPage` / `AniCutYPage` are the number of columns/rows.
  - The current frame is `+0x184`; frame range is
    `AnimationMinPage..AnimationMaxPage`.
  - `AnimationDelay` is the frame duration in seconds.
  - `AnimationLoopCnt` is the loop count; `-1` loops forever. When the loop
    count is exhausted the element sets `visible = false`.
  - Frame selection: `x = frame % XPage`, `y = frame / XPage`; the source rect
    is the corresponding cell.
- `UseRotation true`:
  - Draws with `FUN_004777b0`, using `Rotation` (degrees) and
    `RotationCenter`.
- `ExternalImageFile`:
  - Loads a standalone image (observed `ui/CI.png`) instead of using the
    guimat alias. It can still be animated with the same sheet parameters.
- `RasterOp` controls how the texture fills the destination:
  `split` is 9-slice, `stretch` stretches, `wrap` tiles.

---

## `Button` — `0x436520`

Derives from `Static`; therefore supports `Caption`, `Text`, `ToolTip`,
`MultiLine`, `RichText`, font and texture-slot keys.

State selection:

```text
disabled  -> slot 3   (or slot 0 if fewer than 4 slots)
normal    -> slot 0
mouse-over-> slot 2
pressed   -> slot 1
```

- `+0x211` is the pressed flag.
- `+9` is the hover flag.
- `+0xcc` is the enabled flag.
- When pressed, the text rect is shifted vertically by ±1 px. `DownTextUp`
  chooses the direction.
- If the button has an icon (`Icon`), the text area is shrunk by the icon
  square on the left.
- `Group` is a string used for radio/tab grouping.
- `ToggleButton true` makes the button latch instead of acting as a one-shot.
- `ButtonSound` is a numeric sound id.
- An element named `ok` sets the Enter-key flag `+0x214`; named `cancel` sets
  the Esc-key flag `+0x215`.
- Click dispatch calls vtable `+0x7c` and sends UI event `0x101`.
- `Icon` draws an icon layer through a separate overlay pass (slot `+0x10`).
- Pressing the button captures the mouse; release outside cancels.

---

## `CheckBox` / `RadioButton` — draw `0x437a20`

`CheckBox` derives from `Button`; `RadioButton` derives from `CheckBox`.

- Draws the current state texture (same slot mapping as `Button`).
- Draws the check mark texture at `+0x2a4`.
- If checked (`+0x2a0`), draws a second texture from slot 1.
- Draws the caption with `FUN_0047b7d0`.
- `Group` controls radio-button exclusivity: selecting one radio button clears
  the others in the same group.
- `RadioButton` visually differs only by its checked texture and group
  behaviour.

---

## `GaugeBar` — `0x448900`

Derived from `Static`, so it can also carry a `Caption`/`Text`.

- `BarRange min max`, `BarPos` current value.
- `BarDirection`:
  - `right` (0): fill grows left → right.
  - `left` (1): fill grows right → left.
  - `up` (2): fill grows bottom → top.
  - `down` (3): fill grows top → bottom.
- Texture slots:
  - slot 0 = background/empty bar
  - slot 1 = fill bar
  - slot 2 = frame/overlay
  - additional slot for tick marks
- `BarOutSpace` is a four-value inset/margin (`l t r b`) for the fill area.
- `noFrame true` suppresses the frame texture.
- `text_type`:
  - `rate_fraction` → display as a fraction
  - `rate_percent` → display as a percentage
- `text_postfix` is appended to the numeric text.
- `BarSpark` and `BarFlash` are animated overlay sprites:
  `alias x y w h` (the alias is parsed as a string, followed by four ints).
- `BarFlashFadeoutRate` is the fade speed.
- `FillDrawMethod` selects the fill algorithm (observed values `0`, `1`).

The draw function also supports a “bar flash” that fades and optionally hides
the gauge when it reaches zero.

---

## `Slider` — `0x4899b0`

- Slot 0 = track, slot 1 = knob.
- `range min max` sets the value range.
- `value` sets the initial value.
- Knob position is computed from value/range over the track rect.
- `MouseMoveLock true` makes the knob follow the mouse while dragging without
  needing to stay inside the bounds.
- While dragging, the knob uses the pressed state and 0.8 alpha.
- Clicking the track jumps the knob to that position.

---

## `EditBox` — `0x441e20`

Derived from `CGuiElement`; `ImeEditBox` and `ImeNotePadEditBox` derive from it.

- Draws the box background (slot 0) and an inner text area (slot 1).
- `DrawBox` controls whether the box art is drawn.
- `BorderWidth` insets the inner text rect.
- `Spacing` adds horizontal text padding.
- `SelBkColor`, `SelColor`, `CaretColor` control selection/caret colors.
- Maintains a text string, a caret position and a selection anchor.
- When a selection exists, the selected range is drawn by inverting the text
  color, not by drawing a solid rectangle.
- `Password true` renders a mask character instead of the text.
- `Range` limits the number of characters.
- `multiline` enables wrapping/line breaks.
- `indicator` shows a small IME/status indicator.
- Placeholder/empty text is drawn when the string is empty.
- IME composition is drawn in a separate pass (`FUN_004a2a20`).
- Keyboard handling covers arrows, Home/End, PageUp/PageDown, Delete,
  Backspace, Ctrl+A, Ctrl+C/V/X, Enter and Tab.
- Focus is explicit: `+0x0a` is set by the focus hooks (`+0x24`/`+0x28`).

---

## `ListBox` — `0x4564e0`

- Slot 0 = background, slot 1 = item background, slot 2 = scrollbar.
- Items are child element objects stored in a vector at `+0x3c0`.
- Each item can be a plain text row or a custom panel (see `ListView`).
- Selection:
  - `+0x3b4` selected index
  - `+0x3b8` anchor index
  - shift-click extends the range
  - `DrawSelectBoxLast` controls whether the selection box is drawn after the
    items
- `leftScrollbar` moves the scrollbar to the left.
- `ScrollBarWidth` sets scrollbar thickness.
- `borderWidth`, `spacing`, `itemSpacing` control padding.
- `RichText` enables markup in item text.
- `RenderMethod` selects the item rendering path.
- Handles wheel, scrollbar drag and keyboard navigation.

---

## `ListView` — `0x458840`

Derives from `ListBox`.

- Adds `itemSize w h` and `itemPanel <path>`.
- `itemPanel` loads another `.pane` and instantiates it per row; this is how
  inventory slots, quest rows, guild member rows, chat rows, etc. are built.
- The list can arrange items in a grid: `itemSize` defines the cell, and the
  view computes the number of columns from the client width.
- Draws a header/background and then the visible row templates.
- Handles scrolling, mouse wheel, selection and keyboard navigation.
- `TreeView` derives from `ListView` and adds indentation/expand-collapse
  behaviour.
- `CustomListView` is a separate implementation with the same core keys plus
  `itemSize`/`itemPanel`.

---

## `ComboBox` — `0x437ee0`

Derives from `Button`.

- Draws the button face in the normal/pressed/hover state.
- `dropbox_length` controls the maximum number of visible drop-down items.
- The drop-down is a list of child elements with their own rects; it scrolls
  and highlights the selected row.
- It reuses the button's `Caption`, `Text`, `Group`, icon and sound keys.
- Opening/closing plays the button/UI sounds.

---

## `IconButton` — `0x42c550`

Derives from `Button`.

- `IconTexture` — alias for the icon atlas/page.
- `IconTextureSlot` — slot index in that atlas.
- `IconSlotIndex` — which icon cell/frame.
- `IconSlotType` — icon rendering mode.
- Draws the button background using the normal button state path, then the icon
  on top.
- Supports the same state textures and sounds as `Button`.

---

## `ImageFont` — `0x44baf0`

Bitmap-font text element.

- `FontHeight` — height of one glyph.
- `Margin x y` — padding around the text.
- `WidthScale` — horizontal glyph scale.
- `FontPreAlise` — pre-rendered alias/atlas set name.
- `TextValue` — text to render.
- The glyph atlas is split into pages; the draw function maps character codes
  to glyph rects and blits them one by one.
- Used for stylized lettering/logos where a normal TTF font is not desired.

---

## `ImeNotePad` — `0x4506c0`

A multi-line text area with IME support, independent of `EditBox`.

- `leftScrollbar`, `Wrap`, `RichText`
- `borderWidth`, `spacing`, `itemSpacing`
- `TestString` is the initial/placeholder text.
- Used by guild notices and similar long-text editors.

---

## `Mesh` (`MoMesh`) — `0x46d480`

Renders a 3D mesh in the UI.

- `MoFile` — mesh file.
- `Thickness` — line/edge thickness (int).
- `ParticleSize` — particle size (int).
- The draw function delegates to the 3D renderer; layout uses the same absolute
  rect.

---

## `BalloonBox` / `TooltipBox` — `0x48b1e0` (Static handler)

- `BalloonBox` derives from `Static`; `TooltipBox` derives from `BalloonBox`.
- They use the Static draw path with automatic sizing/positioning around the
  anchor element.
- `ToolTip` text and the mouse-enter/leave hooks (`+0x74`/`+0x78`) show and
  hide them.
- Used for item tooltips and chat balloons.
